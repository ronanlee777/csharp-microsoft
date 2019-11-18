setlocal

@if not defined BUILDDATE (
	echo ERROR: Expecting BUILDDATE to be set
	exit /B 1
)

@if not defined BUILDREVISION (
	echo ERROR: Expecting BUILDREVISION to be set
	exit /B 1
)

call %~dp0\..\build\localization\RunLocWorkflow.cmd Daily-%BUILDDATE%.%BUILDREVISION%
