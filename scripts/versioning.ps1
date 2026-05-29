function Get-FieldMouseVersionInfo {
  param(
    [Parameter(Mandatory)]
    [string]$RepoRoot,

    [string]$TagVersion
  )

  $versionFilePath = Join-Path $RepoRoot 'VERSION.txt'
  if (-not (Test-Path -LiteralPath $versionFilePath)) {
    throw "Version file was not found: $versionFilePath"
  }

  $rawVersion = (Get-Content -LiteralPath $versionFilePath -Raw).Trim()
  if ($rawVersion -notmatch '^\d+\.\d+\.\d+$') {
    throw "VERSION.txt must contain a semantic version in the form major.minor.patch. Found '$rawVersion'."
  }

  if ($TagVersion) {
    $normalizedTagVersion = $TagVersion.Trim()
    if ($normalizedTagVersion.StartsWith('refs/tags/')) {
      $normalizedTagVersion = $normalizedTagVersion.Substring(10)
    }

    if ($normalizedTagVersion -notmatch '^v\d+\.\d+\.\d+$') {
      throw "Tag version must be in the form vmajor.minor.patch. Found '$TagVersion'."
    }

    $normalizedTagVersion = $normalizedTagVersion.Substring(1)
    if ($normalizedTagVersion -ne $rawVersion) {
      throw "VERSION.txt ($rawVersion) does not match the requested tag version ($normalizedTagVersion)."
    }
  }

  $versionParts = $rawVersion.Split('.')
  $windowsVersion = "$($versionParts[0]).$($versionParts[1]).$($versionParts[2]).0"

  return [pscustomobject]@{
    SemanticVersion = $rawVersion
    TagVersion = "v$rawVersion"
    WindowsVersion = $windowsVersion
    WindowsVersionCommas = ($windowsVersion -replace '\.', ',')
    VersionFilePath = $versionFilePath
  }
}
