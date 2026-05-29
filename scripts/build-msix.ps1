param(
  [ValidateSet('Debug', 'Release')]
  [string]$Configuration = 'Release',

  [ValidateSet('MSVC', 'MinGW')]
  [string]$Toolchain = 'MSVC',

  [string]$PackageIdentityName = $env:FIELD_MOUSE_MSIX_IDENTITY_NAME,

  [string]$PackagePublisher = $env:FIELD_MOUSE_MSIX_PUBLISHER,

  [string]$PackageDisplayName = 'Field Mouse',

  [string]$PublisherDisplayName = 'Field Mouse',

  [string]$PackageDescription = 'Field Mouse',

  [string]$MinimumPlatformVersion = '10.0.19041.0',

  [string]$MaxPlatformVersionTested = '10.0.26100.0',

  [string]$VCLibsMinimumVersion = '14.0.30704.0'
)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptRoot
. (Join-Path $scriptRoot 'versioning.ps1')
$versionInfo = Get-FieldMouseVersionInfo -RepoRoot $repoRoot

if (-not $PackageIdentityName) {
  throw 'Package identity name is required. Pass -PackageIdentityName or set FIELD_MOUSE_MSIX_IDENTITY_NAME.'
}

if (-not $PackagePublisher) {
  throw 'Package publisher is required. Pass -PackagePublisher or set FIELD_MOUSE_MSIX_PUBLISHER.'
}

function Find-FieldMouseWindowsSdkTool {
  param([Parameter(Mandatory)][string]$ToolName)

  $sdkRoots = @(
    (Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Windows Kits\10\bin'),
    (Join-Path ([Environment]::GetFolderPath('ProgramFiles')) 'Windows Kits\10\bin')
  )

  foreach ($sdkRoot in $sdkRoots) {
    if (-not (Test-Path -LiteralPath $sdkRoot)) {
      continue
    }

    $candidate = Get-ChildItem -LiteralPath $sdkRoot -Directory -ErrorAction SilentlyContinue |
      Sort-Object Name -Descending |
      ForEach-Object { Join-Path $_.FullName "x64\$ToolName.exe" } |
      Where-Object { Test-Path -LiteralPath $_ } |
      Select-Object -First 1

    if ($candidate) {
      return (Resolve-Path -LiteralPath $candidate).Path
    }
  }

  throw "Windows SDK tool $ToolName.exe was not found."
}

function New-FieldMousePngFromIcon {
  param(
    [Parameter(Mandatory)][string]$IconPath,
    [Parameter(Mandatory)][string]$OutputPath,
    [Parameter(Mandatory)][int]$Width,
    [Parameter(Mandatory)][int]$Height
  )

  Add-Type -AssemblyName System.Drawing
  $icon = $null
  $bitmap = $null
  $resized = $null

  try {
    $iconSize = [Math]::Max($Width, $Height)
    $icon = New-Object System.Drawing.Icon($IconPath, $iconSize, $iconSize)
    $bitmap = $icon.ToBitmap()
    $resized = New-Object System.Drawing.Bitmap($bitmap, $Width, $Height)
    $resized.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
  } finally {
    if ($resized) { $resized.Dispose() }
    if ($bitmap) { $bitmap.Dispose() }
    if ($icon) { $icon.Dispose() }
  }
}

$artifactDir = Join-Path $repoRoot 'artifacts'
$stagingDir = Join-Path $artifactDir "staging\FieldMouse-$($versionInfo.TagVersion)"
if (-not (Test-Path -LiteralPath $stagingDir)) {
  & (Join-Path $scriptRoot 'package-windows.ps1') -Configuration $Configuration -Toolchain $Toolchain
  if ($LASTEXITCODE -ne 0) {
    throw 'The release staging bundle could not be created.'
  }
}

$packageRoot = Join-Path $artifactDir "msix\FieldMouse-$($versionInfo.TagVersion)"
if (Test-Path -LiteralPath $packageRoot) {
  Remove-Item -LiteralPath $packageRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $packageRoot -Force | Out-Null
Copy-Item -Path (Join-Path $stagingDir '*') -Destination $packageRoot -Recurse -Force

$excludedPackageFiles = @(
  'vc_redist.x64.exe'
)

foreach ($excludedFile in $excludedPackageFiles) {
  $candidate = Join-Path $packageRoot $excludedFile
  if (Test-Path -LiteralPath $candidate) {
    Remove-Item -LiteralPath $candidate -Force
  }
}

$assetsDir = Join-Path $packageRoot 'Assets'
New-Item -ItemType Directory -Path $assetsDir -Force | Out-Null
$iconPath = Join-Path $repoRoot 'assets\icons\field-mouse.ico'
New-FieldMousePngFromIcon -IconPath $iconPath -OutputPath (Join-Path $assetsDir 'StoreLogo.png') -Width 50 -Height 50
New-FieldMousePngFromIcon -IconPath $iconPath -OutputPath (Join-Path $assetsDir 'Square44x44Logo.png') -Width 44 -Height 44
New-FieldMousePngFromIcon -IconPath $iconPath -OutputPath (Join-Path $assetsDir 'Square150x150Logo.png') -Width 150 -Height 150
New-FieldMousePngFromIcon -IconPath $iconPath -OutputPath (Join-Path $assetsDir 'Wide310x150Logo.png') -Width 310 -Height 150
New-FieldMousePngFromIcon -IconPath $iconPath -OutputPath (Join-Path $assetsDir 'Square310x310Logo.png') -Width 310 -Height 310

$manifestTemplatePath = Join-Path $repoRoot 'packaging\msix\AppxManifest.xml.in'
$manifestPath = Join-Path $packageRoot 'AppxManifest.xml'
$manifestText = Get-Content -LiteralPath $manifestTemplatePath -Raw
$replacements = @{
  '@PACKAGE_IDENTITY_NAME@' = $PackageIdentityName
  '@PACKAGE_PUBLISHER@' = $PackagePublisher
  '@PACKAGE_VERSION@' = $versionInfo.WindowsVersion
  '@PACKAGE_DISPLAY_NAME@' = $PackageDisplayName
  '@PUBLISHER_DISPLAY_NAME@' = $PublisherDisplayName
  '@PACKAGE_DESCRIPTION@' = $PackageDescription
  '@MINIMUM_PLATFORM_VERSION@' = $MinimumPlatformVersion
  '@MAX_PLATFORM_VERSION_TESTED@' = $MaxPlatformVersionTested
  '@VCLIBS_MIN_VERSION@' = $VCLibsMinimumVersion
}

foreach ($replacement in $replacements.GetEnumerator()) {
  $manifestText = $manifestText.Replace($replacement.Key, $replacement.Value)
}

Set-Content -LiteralPath $manifestPath -Value $manifestText -Encoding utf8NoBOM

$makeAppxPath = Find-FieldMouseWindowsSdkTool -ToolName 'makeappx'
$outputPath = if ($Configuration -eq 'Release') {
  Join-Path $artifactDir "FieldMouse-$($versionInfo.TagVersion).msix"
} else {
  Join-Path $artifactDir "FieldMouse-$Configuration-$($versionInfo.TagVersion).msix"
}

if (Test-Path -LiteralPath $outputPath) {
  Remove-Item -LiteralPath $outputPath -Force
}

& $makeAppxPath pack /d $packageRoot /p $outputPath /o
if ($LASTEXITCODE -ne 0) {
  throw "makeappx.exe failed with exit code $LASTEXITCODE."
}

Write-Host "Created $outputPath"
