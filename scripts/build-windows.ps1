param(
  [ValidateSet('Debug', 'Release')]
  [string]$Configuration = 'Debug',

  [string]$QtRoot = '',

  [ValidateSet('MSVC', 'MinGW', 'Auto')]
  [string]$Toolchain = 'MSVC',

  [string]$MinGwRoot = '',

  [switch]$Fresh
)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptRoot

function Resolve-FieldMouseQtRoot {
  param(
    [string]$RequestedQtRoot,
    [string]$RequestedToolchain
  )

  if ($RequestedQtRoot) {
    return (Resolve-Path -LiteralPath $RequestedQtRoot).Path
  }

  $candidateRoots = switch ($RequestedToolchain) {
    'MSVC' { @('C:\Qt\6.11.1\msvc2022_64') }
    'MinGW' { @('C:\Qt\6.11.1\mingw_64') }
    default { @('C:\Qt\6.11.1\msvc2022_64', 'C:\Qt\6.11.1\mingw_64') }
  }

  foreach ($candidateRoot in $candidateRoots) {
    if (Test-Path -LiteralPath $candidateRoot) {
      return (Resolve-Path -LiteralPath $candidateRoot).Path
    }
  }

  if ($RequestedToolchain -eq 'MSVC') {
    throw 'Qt 6.11.1 MSVC kit was not found at C:\Qt\6.11.1\msvc2022_64. Add the Qt 6.11.1 MSVC 2022 64-bit component, or pass -QtRoot to that kit.'
  }

  if ($RequestedToolchain -eq 'MinGW') {
    throw 'Qt 6.11.1 MinGW kit was not found at C:\Qt\6.11.1\mingw_64. Add the Qt 6.11.1 MinGW 64-bit component, or pass -QtRoot to that kit.'
  }

  throw 'Qt 6.11.1 was not found. Install an MSVC or MinGW kit under C:\Qt\6.11.1, or pass -QtRoot.'
}

function Add-FieldMousePathPrefix {
  param([string[]]$Paths)

  $existingParts = $env:PATH -split ';'
  $prefixParts = foreach ($path in $Paths) {
    if ($path -and (Test-Path -LiteralPath $path)) {
      (Resolve-Path -LiteralPath $path).Path
    }
  }

  $allParts = @($prefixParts) + @($existingParts)
  $dedupedParts = New-Object System.Collections.Generic.List[string]
  foreach ($part in $allParts) {
    if ($part -and -not $dedupedParts.Contains($part)) {
      [void]$dedupedParts.Add($part)
    }
  }

  $env:PATH = $dedupedParts -join ';'
}

function ConvertTo-FieldMouseCMakePath {
  param([string]$Path)

  return $Path.Replace('\', '/')
}

function Invoke-FieldMouseNativeCommand {
  param(
    [string]$FilePath,
    [string[]]$Arguments
  )

  & $FilePath @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($Arguments -join ' ')"
  }
}

function Find-FieldMouseWindowsSdkTool {
  param([string]$ToolName)

  $sdkRoots = @(
    (Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Windows Kits\10\bin'),
    (Join-Path ([Environment]::GetFolderPath('ProgramFiles')) 'Windows Kits\10\bin')
  )

  $candidatePaths = foreach ($sdkRoot in $sdkRoots) {
    if (Test-Path -LiteralPath $sdkRoot) {
      Get-ChildItem -LiteralPath $sdkRoot -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending |
        ForEach-Object {
          Join-Path $_.FullName "x64\$ToolName.exe"
        }
    }
  }

  foreach ($candidatePath in $candidatePaths) {
    if (Test-Path -LiteralPath $candidatePath) {
      return (Resolve-Path -LiteralPath $candidatePath).Path
    }
  }

  throw "Windows SDK tool $ToolName.exe was not found. Install the Windows 10 or 11 SDK."
}

function Get-FieldMouseWindowsSdkRoot {
  $sdkRoots = @(
    (Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Windows Kits\10'),
    (Join-Path ([Environment]::GetFolderPath('ProgramFiles')) 'Windows Kits\10')
  )

  foreach ($sdkRoot in $sdkRoots) {
    if (Test-Path -LiteralPath $sdkRoot) {
      return (Resolve-Path -LiteralPath $sdkRoot).Path
    }
  }

  throw 'Windows 10 or 11 SDK root was not found.'
}

function Get-FieldMouseWindowsSdkVersion {
  param([string]$SdkRoot)

  $libRoot = Join-Path $SdkRoot 'Lib'
  $sdkVersion = Get-ChildItem -LiteralPath $libRoot -Directory -ErrorAction SilentlyContinue |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'um\x64\kernel32.lib') } |
    Sort-Object Name -Descending |
    Select-Object -First 1

  if (-not $sdkVersion) {
    throw "No x64 Windows SDK library version with kernel32.lib was found under $libRoot."
  }

  return $sdkVersion.Name
}

function Add-FieldMouseWindowsSdkEnvironment {
  $sdkRoot = Get-FieldMouseWindowsSdkRoot
  $sdkVersion = Get-FieldMouseWindowsSdkVersion -SdkRoot $sdkRoot
  $sdkBinPath = Join-Path $sdkRoot "bin\$sdkVersion\x64"
  $sdkIncludePaths = @(
    (Join-Path $sdkRoot "Include\$sdkVersion\ucrt"),
    (Join-Path $sdkRoot "Include\$sdkVersion\shared"),
    (Join-Path $sdkRoot "Include\$sdkVersion\um"),
    (Join-Path $sdkRoot "Include\$sdkVersion\winrt"),
    (Join-Path $sdkRoot "Include\$sdkVersion\cppwinrt")
  )
  $sdkLibPaths = @(
    (Join-Path $sdkRoot "Lib\$sdkVersion\ucrt\x64"),
    (Join-Path $sdkRoot "Lib\$sdkVersion\um\x64")
  )

  Add-FieldMousePathPrefix -Paths @($sdkBinPath)
  $env:INCLUDE = (($sdkIncludePaths | Where-Object { Test-Path -LiteralPath $_ }) + @($env:INCLUDE)) -join ';'
  $env:LIB = (($sdkLibPaths | Where-Object { Test-Path -LiteralPath $_ }) + @($env:LIB)) -join ';'
  $env:WindowsSdkDir = "$sdkRoot\"
  $env:WindowsSDKVersion = "$sdkVersion\"

  return [pscustomobject]@{
    Root = $sdkRoot
    Version = $sdkVersion
    BinPath = $sdkBinPath
    RcPath = Join-Path $sdkBinPath 'rc.exe'
    MtPath = Join-Path $sdkBinPath 'mt.exe'
  }
}

function Import-FieldMouseBatchEnvironment {
  param(
    [string]$BatchPath,
    [string[]]$Arguments
  )

  $quotedBatchPath = '"' + $BatchPath + '"'
  $batchArguments = $Arguments -join ' '
  $command = "$quotedBatchPath $batchArguments > nul && set"
  $environmentLines = & cmd.exe /s /c $command

  foreach ($environmentLine in $environmentLines) {
    $separatorIndex = $environmentLine.IndexOf('=')
    if ($separatorIndex -le 0) {
      continue
    }

    $name = $environmentLine.Substring(0, $separatorIndex)
    $value = $environmentLine.Substring($separatorIndex + 1)
    Set-Item -Path "Env:$name" -Value $value
  }
}

function Enable-FieldMouseMsvcEnvironment {
  if ($env:VSCMD_ARG_TGT_ARCH -eq 'x64' -and (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    return
  }

  $programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
  $vswherePath = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
  $candidateDevCmdPaths = New-Object System.Collections.Generic.List[string]

  if (Test-Path -LiteralPath $vswherePath) {
    $visualStudioPath = & $vswherePath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($visualStudioPath) {
      $candidateDevCmdPath = Join-Path $visualStudioPath 'Common7\Tools\VsDevCmd.bat'
      if (Test-Path -LiteralPath $candidateDevCmdPath) {
        [void]$candidateDevCmdPaths.Add($candidateDevCmdPath)
      }
    }
  }

  $visualStudioRoots = @(
    (Join-Path ([Environment]::GetFolderPath('ProgramFiles')) 'Microsoft Visual Studio'),
    (Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio')
  )

  foreach ($visualStudioRoot in $visualStudioRoots) {
    if (-not (Test-Path -LiteralPath $visualStudioRoot)) {
      continue
    }

    Get-ChildItem -LiteralPath $visualStudioRoot -Directory -ErrorAction SilentlyContinue |
      ForEach-Object {
        Get-ChildItem -LiteralPath $_.FullName -Directory -ErrorAction SilentlyContinue
      } |
      ForEach-Object {
        $candidateDevCmdPath = Join-Path $_.FullName 'Common7\Tools\VsDevCmd.bat'
        if ((Test-Path -LiteralPath $candidateDevCmdPath) -and -not $candidateDevCmdPaths.Contains($candidateDevCmdPath)) {
          [void]$candidateDevCmdPaths.Add($candidateDevCmdPath)
        }
      }
  }

  $devCmdPath = $candidateDevCmdPaths |
    Sort-Object -Descending |
    Select-Object -First 1

  if (-not $devCmdPath) {
    throw 'Visual Studio C++ tools were not found. Install Desktop development with C++, or run from an x64 Native Tools prompt.'
  }

  Import-FieldMouseBatchEnvironment -BatchPath $devCmdPath -Arguments @('-arch=x64', '-host_arch=x64')

  if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw "VsDevCmd.bat was found at $devCmdPath, but cl.exe was not available afterward. Verify the Visual Studio C++ workload is installed."
  }
}

function Resolve-FieldMouseMinGwRoot {
  param([string]$RequestedMinGwRoot)

  if ($RequestedMinGwRoot) {
    return (Resolve-Path -LiteralPath $RequestedMinGwRoot).Path
  }

  $qtInstallRoot = Split-Path -Parent (Split-Path -Parent $resolvedQtRoot)
  $toolsRoot = Join-Path $qtInstallRoot 'Tools'
  $preferredRoot = Join-Path $toolsRoot 'mingw1310_64'
  if (Test-Path -LiteralPath $preferredRoot) {
    return (Resolve-Path -LiteralPath $preferredRoot).Path
  }

  $candidateRoot = Get-ChildItem -LiteralPath $toolsRoot -Directory -Filter 'mingw*_64' -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending |
    Select-Object -First 1

  if ($candidateRoot) {
    return $candidateRoot.FullName
  }

  throw "Qt MinGW tools were not found under $toolsRoot. Pass -MinGwRoot."
}

$resolvedQtRoot = Resolve-FieldMouseQtRoot -RequestedQtRoot $QtRoot -RequestedToolchain $Toolchain
$qtConfigPath = Join-Path $resolvedQtRoot 'lib\cmake\Qt6\Qt6Config.cmake'
$windeployQtPath = Join-Path $resolvedQtRoot 'bin\windeployqt.exe'

if (-not (Test-Path -LiteralPath $qtConfigPath)) {
  throw "Qt6Config.cmake was not found at $qtConfigPath."
}

if (-not (Test-Path -LiteralPath $windeployQtPath)) {
  throw "windeployqt.exe was not found at $windeployQtPath."
}

$effectiveToolchain = $Toolchain
$qtRootName = Split-Path -Leaf $resolvedQtRoot
if ($effectiveToolchain -eq 'Auto') {
  if ($qtRootName -match 'mingw') {
    $effectiveToolchain = 'MinGW'
  } elseif ($qtRootName -match 'msvc') {
    $effectiveToolchain = 'MSVC'
  } else {
    throw "Could not infer the compiler family from Qt root '$resolvedQtRoot'. Pass -Toolchain MSVC or -Toolchain MinGW."
  }
}

if ($effectiveToolchain -eq 'MSVC' -and $qtRootName -match 'mingw') {
  throw "The selected Qt kit '$resolvedQtRoot' is a MinGW kit, but -Toolchain MSVC was requested. Install/select an MSVC Qt kit such as C:\Qt\6.11.1\msvc2022_64."
}

if ($effectiveToolchain -eq 'MinGW' -and $qtRootName -match 'msvc') {
  throw "The selected Qt kit '$resolvedQtRoot' is an MSVC kit, but -Toolchain MinGW was requested. Select C:\Qt\6.11.1\mingw_64 or use -Toolchain MSVC."
}

$cmakePresetPrefix = if ($effectiveToolchain -eq 'MSVC') { 'windows-msvc' } else { 'windows-mingw' }
$cmakeConfigureArgs = @('--preset', "$cmakePresetPrefix-$($Configuration.ToLowerInvariant())")

if ($effectiveToolchain -eq 'MSVC') {
  Enable-FieldMouseMsvcEnvironment
  $windowsSdk = Add-FieldMouseWindowsSdkEnvironment
  $windowsSdkRcPath = $windowsSdk.RcPath
  $windowsSdkMtPath = $windowsSdk.MtPath
  Add-FieldMousePathPrefix -Paths @((Join-Path $resolvedQtRoot 'bin'))
  $cmakeConfigureArgs += @(
    "-DCMAKE_RC_COMPILER=$(ConvertTo-FieldMouseCMakePath $windowsSdkRcPath)",
    "-DCMAKE_MT=$(ConvertTo-FieldMouseCMakePath $windowsSdkMtPath)",
    "-DCMAKE_SYSTEM_VERSION=$($windowsSdk.Version)"
  )
} else {
  $resolvedMinGwRoot = Resolve-FieldMouseMinGwRoot -RequestedMinGwRoot $MinGwRoot
  $ninjaPath = Join-Path (Split-Path -Parent (Split-Path -Parent $resolvedQtRoot)) 'Tools\Ninja\ninja.exe'
  $cmakeBinPath = Join-Path (Split-Path -Parent (Split-Path -Parent $resolvedQtRoot)) 'Tools\CMake_64\bin'

  if (-not (Test-Path -LiteralPath $ninjaPath)) {
    $ninjaCommand = Get-Command ninja.exe -ErrorAction SilentlyContinue
    if (-not $ninjaCommand) {
      throw 'ninja.exe was not found. Install Ninja or pass a Qt installation that includes Tools\Ninja.'
    }
    $ninjaPath = $ninjaCommand.Source
  }

  Add-FieldMousePathPrefix -Paths @(
    (Join-Path $resolvedQtRoot 'bin'),
    (Join-Path $resolvedMinGwRoot 'bin'),
    (Split-Path -Parent $ninjaPath),
    $cmakeBinPath
  )

  $cmakeConfigureArgs += @(
    "-DCMAKE_CXX_COMPILER=$(ConvertTo-FieldMouseCMakePath (Join-Path $resolvedMinGwRoot 'bin\g++.exe'))",
    "-DCMAKE_RC_COMPILER=$(ConvertTo-FieldMouseCMakePath (Join-Path $resolvedMinGwRoot 'bin\windres.exe'))",
    "-DCMAKE_MAKE_PROGRAM=$(ConvertTo-FieldMouseCMakePath $ninjaPath)"
  )
}

$env:QT_ROOT_DIR = $resolvedQtRoot
$binaryDir = Join-Path $repoRoot "obj\$cmakePresetPrefix-$($Configuration.ToLowerInvariant())"

if ($Fresh -and (Test-Path -LiteralPath $binaryDir)) {
  Remove-Item -LiteralPath $binaryDir -Recurse -Force
}

Write-Host "Repository: $repoRoot"
Write-Host "Configuration: $Configuration"
Write-Host "Qt root: $resolvedQtRoot"
Write-Host "Toolchain: $effectiveToolchain"

Push-Location $repoRoot
try {
  Invoke-FieldMouseNativeCommand -FilePath 'cmake' -Arguments $cmakeConfigureArgs
  Invoke-FieldMouseNativeCommand -FilePath 'cmake' -Arguments @('--build', $binaryDir)
} finally {
  Pop-Location
}