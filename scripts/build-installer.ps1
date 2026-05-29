param(
  [ValidateSet('Debug', 'Release')]
  [string]$Configuration = 'Release',

  [ValidateSet('MSVC', 'MinGW')]
  [string]$Toolchain = 'MSVC',

  [string]$InnoSetupCompiler = '',

  [string]$AppId = '{{B762D3E2-211B-4B6C-B1DB-2A67A86933F3}}'
)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptRoot
. (Join-Path $scriptRoot 'versioning.ps1')
$versionInfo = Get-FieldMouseVersionInfo -RepoRoot $repoRoot

function Resolve-FieldMouseInnoCompiler {
  param([string]$RequestedPath)

  if ($RequestedPath) {
    if (-not (Test-Path -LiteralPath $RequestedPath)) {
      throw "Inno Setup compiler was not found: $RequestedPath"
    }

    return (Resolve-Path -LiteralPath $RequestedPath).Path
  }

  $command = Get-Command iscc.exe -ErrorAction SilentlyContinue
  if ($command) {
    return $command.Source
  }

  $candidatePaths = @(
    'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
    'C:\Program Files\Inno Setup 6\ISCC.exe'
  )

  foreach ($candidatePath in $candidatePaths) {
    if (Test-Path -LiteralPath $candidatePath) {
      return (Resolve-Path -LiteralPath $candidatePath).Path
    }
  }

  throw 'ISCC.exe was not found. Install Inno Setup 6 or pass -InnoSetupCompiler.'
}

$artifactDir = Join-Path $repoRoot 'artifacts'
$stagingDir = Join-Path $artifactDir "staging\FieldMouse-$($versionInfo.TagVersion)"
if (-not (Test-Path -LiteralPath $stagingDir)) {
  & (Join-Path $scriptRoot 'package-windows.ps1') -Configuration $Configuration -Toolchain $Toolchain
  if ($LASTEXITCODE -ne 0) {
    throw 'The release staging bundle could not be created.'
  }
}

$innoCompiler = Resolve-FieldMouseInnoCompiler -RequestedPath $InnoSetupCompiler
$scriptPath = Join-Path $repoRoot 'packaging\windows\FieldMouse.iss'
$outputBaseName = if ($Configuration -eq 'Release') { "FieldMouse-setup-$($versionInfo.TagVersion)" } else { "FieldMouse-setup-$Configuration-$($versionInfo.TagVersion)" }

$innoArguments = @(
  "/DAppVersion=$($versionInfo.SemanticVersion)",
  "/DAppVersionTag=$($versionInfo.TagVersion)",
  "/DAppId=$AppId",
  "/DSourceDir=$stagingDir",
  "/DOutputDir=$artifactDir",
  "/DOutputBaseFilename=$outputBaseName",
  "/DSetupIconFile=$(Join-Path $repoRoot 'assets\icons\field-mouse.ico')",
  "/DLicenseFile=$(Join-Path $repoRoot 'LICENSE')",
  $scriptPath
)

& $innoCompiler @innoArguments

if ($LASTEXITCODE -ne 0) {
  throw "Inno Setup failed with exit code $LASTEXITCODE."
}

$installerPath = Join-Path $artifactDir "$outputBaseName.exe"
if (-not (Test-Path -LiteralPath $installerPath)) {
  throw "Expected installer output was not created: $installerPath"
}

Write-Host "Created $installerPath"
