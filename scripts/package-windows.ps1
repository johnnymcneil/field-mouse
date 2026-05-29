param(
  [ValidateSet('Debug', 'Release')]
  [string]$Configuration = 'Release',

  [ValidateSet('MSVC', 'MinGW')]
  [string]$Toolchain = 'MSVC',

  [switch]$SkipArchive,

  [switch]$UseExistingStaging
)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptRoot
. (Join-Path $scriptRoot 'versioning.ps1')
$versionInfo = Get-FieldMouseVersionInfo -RepoRoot $repoRoot
$outputDir = Join-Path $repoRoot "bin\$Configuration"
$cmakePresetPrefix = if ($Toolchain -eq 'MSVC') { 'windows-msvc' } else { 'windows-mingw' }
$buildDir = Join-Path $repoRoot "obj\$cmakePresetPrefix-$($Configuration.ToLowerInvariant())"
$cachePath = Join-Path $buildDir 'CMakeCache.txt'
$artifactDir = Join-Path $repoRoot 'artifacts'
$stagingRoot = Join-Path $artifactDir 'staging'
$stagingDir = Join-Path $stagingRoot "FieldMouse-$($versionInfo.TagVersion)"
$archiveName = if ($Configuration -eq 'Release') { "FieldMouse-windows-x64-$($versionInfo.TagVersion).zip" } else { "FieldMouse-windows-x64-$Configuration-$($versionInfo.TagVersion).zip" }
$archivePath = Join-Path $artifactDir $archiveName

$requiredFiles = @(
  'FieldMouse.exe',
  'Qt6Core.dll',
  'Qt6Gui.dll',
  'Qt6Widgets.dll',
  'LICENSE',
  'THIRD_PARTY_NOTICES.md',
  'LGPL_COMPLIANCE.md',
  'LICENSES\LGPL-3.0.txt',
  'LICENSES\GPL-3.0.txt',
  'VERSION.txt',
  'platforms\qwindows.dll'
)

if (-not (Test-Path -LiteralPath $cachePath)) {
  throw "Expected $Toolchain CMake cache was not found: $cachePath. Run ./scripts/build-windows.ps1 -Configuration $Configuration -Toolchain $Toolchain first."
}

$cacheText = Get-Content -LiteralPath $cachePath -Raw

if ($Toolchain -eq 'MSVC') {
  if ($cacheText -match '(?im)^CMAKE_CXX_COMPILER:.*(?:mingw|g\+\+\.exe)' -or $cacheText -match '(?im)^Qt6_DIR:.*mingw') {
    throw 'The release build cache is not an MSVC/Qt MSVC build. Refusing to package MinGW output as the canonical Windows release.'
  }
} else {
  if ($cacheText -notmatch '(?im)^CMAKE_CXX_COMPILER:.*(?:mingw|g\+\+\.exe)' -or $cacheText -notmatch '(?im)^Qt6_DIR:.*mingw') {
    throw 'The build cache does not look like a MinGW Qt build. Refusing to package with -Toolchain MinGW.'
  }
}

foreach ($requiredFile in $requiredFiles) {
  $requiredPath = Join-Path $outputDir $requiredFile
  if (-not (Test-Path -LiteralPath $requiredPath)) {
    throw "Required release file is missing: $requiredPath"
  }
}

if (-not $UseExistingStaging) {
  if (Test-Path -LiteralPath $stagingRoot) {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
  }

  New-Item -ItemType Directory -Path $stagingDir -Force | Out-Null
  Copy-Item -Path (Join-Path $outputDir '*') -Destination $stagingDir -Recurse -Force

  $localOnlyPatterns = @(
    'field-mouse-settings.json',
    'field-mouse-data',
    '*.ilk',
    '*.pdb'
  )

  foreach ($localOnlyPattern in $localOnlyPatterns) {
    Get-ChildItem -LiteralPath $stagingDir -Recurse -Force -Filter $localOnlyPattern -ErrorAction SilentlyContinue |
      Remove-Item -Recurse -Force
  }
} elseif (-not (Test-Path -LiteralPath $stagingDir)) {
  throw "Expected existing staging directory was not found: $stagingDir"
}

if ($SkipArchive) {
  Write-Host "Prepared $stagingDir"
  return
}

if (Test-Path -LiteralPath $archivePath) {
  Remove-Item -LiteralPath $archivePath -Force
}

Compress-Archive -Path (Join-Path $stagingDir '*') -DestinationPath $archivePath -CompressionLevel Optimal

Write-Host "Created $archivePath"