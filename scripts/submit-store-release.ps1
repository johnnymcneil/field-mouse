param(
  [Parameter(Mandatory)]
  [string]$ApplicationId,

  [Parameter(Mandatory)]
  [string]$PackagePath,

  [string]$ReleaseNotes = '',

  [string]$AccessToken = $env:PARTNER_CENTER_ACCESS_TOKEN,

  [string]$TenantId = $env:PARTNER_CENTER_TENANT_ID,

  [string]$ClientId = $env:PARTNER_CENTER_CLIENT_ID,

  [string]$ClientSecret = $env:PARTNER_CENTER_CLIENT_SECRET,

  [int]$StatusPollSeconds = 30,

  [int]$MaxStatusPolls = 40
)

$ErrorActionPreference = 'Stop'

function Get-PartnerCenterAccessToken {
  param(
    [string]$ExistingToken,
    [string]$Tenant,
    [string]$Client,
    [string]$Secret
  )

  if ($ExistingToken) {
    return $ExistingToken
  }

  if (-not ($Tenant -and $Client -and $Secret)) {
    throw 'Provide -AccessToken or set TenantId, ClientId, and ClientSecret.'
  }

  $tokenResponse = Invoke-RestMethod 
    -Method Post 
    -Uri "https://login.microsoftonline.com/$Tenant/oauth2/token" 
    -Body @{
      grant_type = 'client_credentials'
      client_id = $Client
      client_secret = $Secret
      resource = 'https://manage.devcenter.microsoft.com'
    } 
    -ContentType 'application/x-www-form-urlencoded'

  if (-not $tokenResponse.access_token) {
    throw 'Failed to acquire a Partner Center access token.'
  }

  return $tokenResponse.access_token
}

function Invoke-PartnerCenterJson {
  param(
    [Parameter(Mandatory)][string]$Method,
    [Parameter(Mandatory)][string]$Uri,
    [Parameter(Mandatory)][string]$Token,
    [object]$Body
  )

  $headers = @{ Authorization = "Bearer $Token" }
  if ($PSBoundParameters.ContainsKey('Body')) {
    $jsonBody = $Body | ConvertTo-Json -Depth 100
    return Invoke-RestMethod -Method $Method -Uri $Uri -Headers $headers -Body $jsonBody -ContentType 'application/json'
  }

  return Invoke-RestMethod -Method $Method -Uri $Uri -Headers $headers
}

$resolvedPackagePath = (Resolve-Path -LiteralPath $PackagePath).Path
$packageFileName = [IO.Path]::GetFileName($resolvedPackagePath)
$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("field-mouse-store-" + [guid]::NewGuid().ToString('N'))
$zipSourceDir = Join-Path $tempRoot 'submission'
$packagesDir = Join-Path $zipSourceDir 'Packages'
$submissionZipPath = Join-Path $tempRoot 'submission.zip'

New-Item -ItemType Directory -Path $packagesDir -Force | Out-Null
Copy-Item -LiteralPath $resolvedPackagePath -Destination (Join-Path $packagesDir $packageFileName) -Force
Compress-Archive -Path (Join-Path $zipSourceDir '*') -DestinationPath $submissionZipPath -CompressionLevel Optimal

$token = Get-PartnerCenterAccessToken -ExistingToken $AccessToken -Tenant $TenantId -Client $ClientId -Secret $ClientSecret
$baseUri = "https://manage.devcenter.microsoft.com/v1.0/my/applications/$ApplicationId/submissions"

try {
  $submission = Invoke-PartnerCenterJson -Method Post -Uri $baseUri -Token $token
  $submissionData = $submission | ConvertTo-Json -Depth 100 | ConvertFrom-Json -AsHashtable

  $submissionData['applicationPackages'] = @(
    @{
      fileName = "Packages/$packageFileName"
      fileStatus = 'PendingUpload'
      minimumDirectXVersion = 'None'
      minimumSystemRam = 'None'
    }
  )

  if ($ReleaseNotes -and $submissionData.ContainsKey('listings')) {
    foreach ($listingLocale in @($submissionData['listings'].Keys)) {
      $baseListing = $submissionData['listings'][$listingLocale]['baseListing']
      if ($baseListing) {
        $baseListing['releaseNotes'] = $ReleaseNotes
      }
    }
  }

  [void]$submissionData.Remove('fileUploadUrl')
  [void]$submissionData.Remove('statusDetails')
  [void]$submissionData.Remove('status')

  $submissionId = $submissionData['id']
  $updateUri = "$baseUri/$submissionId"
  [void](Invoke-PartnerCenterJson -Method Put -Uri $updateUri -Token $token -Body $submissionData)

  Invoke-WebRequest 
    -Method Put 
    -Uri $submission.fileUploadUrl 
    -InFile $submissionZipPath 
    -Headers @{ 'x-ms-blob-type' = 'BlockBlob' } 
    -ContentType 'application/zip' | Out-Null

  [void](Invoke-PartnerCenterJson -Method Post -Uri "$updateUri/commit" -Token $token)

  $status = $null
  for ($attempt = 0; $attempt -lt $MaxStatusPolls; $attempt++) {
    Start-Sleep -Seconds $StatusPollSeconds
    $status = Invoke-PartnerCenterJson -Method Get -Uri "$updateUri/status" -Token $token
    if ($status.status -ne 'CommitStarted') {
      break
    }
  }

  if (-not $status) {
    throw 'Did not receive a submission status response.'
  }

  if ($status.status -eq 'CommitFailed') {
    $errorJson = $status | ConvertTo-Json -Depth 20
    throw "Partner Center submission commit failed: $errorJson"
  }

  $result = [pscustomobject]@{
    SubmissionId = $submissionId
    Status = $status.status
    StatusDetails = $status.statusDetails
  }

  $result | ConvertTo-Json -Depth 20
} finally {
  if (Test-Path -LiteralPath $tempRoot) {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force
  }
}
