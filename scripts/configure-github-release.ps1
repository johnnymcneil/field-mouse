param(
  [Parameter(Mandatory)]
  [string]$Repository,

  [Parameter(Mandatory)]
  [string]$ArtifactSigningEndpoint,

  [Parameter(Mandatory)]
  [string]$ArtifactSigningAccountName,

  [Parameter(Mandatory)]
  [string]$ArtifactSigningCertificateProfile,

  [string]$AzureClientId,

  [string]$AzureTenantId,

  [string]$AzureSubscriptionId,

  [switch]$PromptForSecrets
)

$ErrorActionPreference = 'Stop'

function Test-FieldMouseCommand {
  param([Parameter(Mandatory)][string]$CommandName)

  return $null -ne (Get-Command $CommandName -ErrorAction SilentlyContinue)
}

function Set-FieldMouseGitHubVariable {
  param(
    [Parameter(Mandatory)][string]$Repo,
    [Parameter(Mandatory)][string]$Name,
    [Parameter(Mandatory)][string]$Value
  )

  gh variable set $Name --repo $Repo --body $Value | Out-Null
}

function Set-FieldMouseGitHubSecret {
  param(
    [Parameter(Mandatory)][string]$Repo,
    [Parameter(Mandatory)][string]$Name,
    [string]$Value
  )

  if (-not $Value) {
    return
  }

  $Value | gh secret set $Name --repo $Repo --body - | Out-Null
}

if (-not (Test-FieldMouseCommand -CommandName 'gh')) {
  throw 'GitHub CLI (gh) is required. Install it and run `gh auth login` first.'
}

$authStatus = gh auth status 2>&1
if ($LASTEXITCODE -ne 0) {
  throw "GitHub CLI is not authenticated. Run `gh auth login` first. Details: $authStatus"
}

if ($PromptForSecrets) {
  if (-not $AzureClientId) {
    $AzureClientId = Read-Host 'AZURE_CLIENT_ID'
  }

  if (-not $AzureTenantId) {
    $AzureTenantId = Read-Host 'AZURE_TENANT_ID'
  }

  if (-not $AzureSubscriptionId) {
    $AzureSubscriptionId = Read-Host 'AZURE_SUBSCRIPTION_ID'
  }
}

$requiredSecrets = @{
  AZURE_CLIENT_ID = $AzureClientId
  AZURE_TENANT_ID = $AzureTenantId
  AZURE_SUBSCRIPTION_ID = $AzureSubscriptionId
}

$missingSecrets = $requiredSecrets.GetEnumerator() |
  Where-Object { [string]::IsNullOrWhiteSpace($_.Value) } |
  Select-Object -ExpandProperty Key

if ($missingSecrets.Count -gt 0) {
  throw "Missing required secret values: $($missingSecrets -join ', '). Pass them as parameters or use -PromptForSecrets."
}

$variables = [ordered]@{
  ARTIFACT_SIGNING_ENDPOINT = $ArtifactSigningEndpoint
  ARTIFACT_SIGNING_ACCOUNT_NAME = $ArtifactSigningAccountName
  ARTIFACT_SIGNING_CERTIFICATE_PROFILE = $ArtifactSigningCertificateProfile
}

foreach ($entry in $variables.GetEnumerator()) {
  Set-FieldMouseGitHubVariable -Repo $Repository -Name $entry.Key -Value $entry.Value
}

foreach ($entry in $requiredSecrets.GetEnumerator()) {
  Set-FieldMouseGitHubSecret -Repo $Repository -Name $entry.Key -Value $entry.Value
}

Write-Host "Configured GitHub signing and release variables/secrets for $Repository"