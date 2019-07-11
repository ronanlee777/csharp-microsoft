[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)] 
    [int]$MinimumExpectedTestsExecutedCount
)


$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$($env:SYSTEM_ACCESSTOKEN)")))"
}

. "$PSScriptRoot/AzurePipelinesHelperScripts.ps1"

Write-Host "Checking test results..."

$queryUri = GetQueryTestRunsUri
Write-Host "queryUri = $queryUri"

$testRuns = Invoke-RestMethod -Uri $queryUri -Method Get -Headers $azureDevOpsRestApiHeaders
[System.Collections.Generic.List[string]]$failingTests = @()
[System.Collections.Generic.List[string]]$unreliableTests = @()

$totalTestsExecutedCount = 0

foreach ($testRun in $testRuns.value)
{
    $totalTestsExecutedCount += $testRun.totalTests

    $testRunResultsUri = "$($testRun.url)/results?api-version=5.0"
    $testResults = Invoke-RestMethod -Uri "$($testRun.url)/results?api-version=5.0" -Method Get -Headers $azureDevOpsRestApiHeaders
        
    foreach ($testResult in $testResults.value)
    {
        $shortTestCaseTitle = $testResult.testCaseTitle -replace "release.[a-zA-Z0-9]+.Windows.UI.Xaml.Tests.MUXControls.",""

        if ($testResult.outcome -eq "Failed")
        {
            if (-not $failingTests.Contains($shortTestCaseTitle))
            {
                $failingTests.Add($shortTestCaseTitle)
            }
        }
        elseif ($testResult.outcome -eq "Warning")
        {
            if (-not $unreliableTests.Contains($shortTestCaseTitle))
            {
                $unreliableTests.Add($shortTestCaseTitle)
            }
        }
    }
}

if ($unreliableTests.Count -gt 0)
{
    Write-Host @"
##vso[task.logissue type=warning;]Unreliable tests:
##vso[task.logissue type=warning;]$($unreliableTests -join "$([Environment]::NewLine)##vso[task.logissue type=warning;]")

"@
}

if ($failingTests.Count -gt 0)
{
    Write-Host @"
##vso[task.logissue type=error;]Failing tests:
##vso[task.logissue type=error;]$($failingTests -join "$([Environment]::NewLine)##vso[task.logissue type=error;]")

"@
}

if($totalTestsExecutedCount -lt $MinimumExpectedTestsExecutedCount)
{
    Write-Host "Expected at least $MinimumExpectedTestsExecutedCount tests to be executed."
    Write-Host "Actual executed test count is: $totalTestsExecutedCount"
    Write-Host "##vso[task.complete result=Failed;]"
}
elseif ($failingTests.Count -gt 0)
{
    Write-Host "At least one test failed."
    Write-Host "##vso[task.complete result=Failed;]"
}
elseif ($unreliableTests.Count -gt 0)
{
    Write-Host "All tests eventually passed, but some initially failed."
    Write-Host "##vso[task.complete result=SucceededWithIssues;]"
}
else
{
    Write-Host "All tests passed."
    Write-Host "##vso[task.complete result=Succeeded;]"
}