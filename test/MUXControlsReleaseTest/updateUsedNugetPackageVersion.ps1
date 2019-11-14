﻿[CmdLetBinding()]
Param(
    [Parameter(mandatory=$true)]
    [string]$newVersion,

    # this string literal is split in two to avoid this script finding it and overwriting itself.
    [string]$currentPackageVersion = "2.1." + "190606001"
)

$scriptDirectory = $script:MyInvocation.MyCommand.Path | Split-Path -Parent
pushd $scriptDirectory

$numFilesReplaced = 0
Get-ChildItem $scriptDirectory -r -File |
  ForEach-Object {
    $path = $_.FullName
    $contents = [System.IO.File]::ReadAllText($path)
    $newContents = $contents.Replace("$currentPackageVersion", "$newVersion")
    if ($contents -ne $newContents) {
      Write-Host "Updating version in $path"
      $newContents | Set-Content $path -Encoding UTF8
      $numFilesReplaced += 1
    }
  }

if ($numFilesReplaced -eq 0)
{
  Write-Error "No files found with '$currentPackageVersion' in them."
  Exit 1
}