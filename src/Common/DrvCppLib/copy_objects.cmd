rem Set ARCH
@echo %_BUILDARCH%

@if /I "%_BUILDARCH%"=="x86" (
        @set ARCH=i386
) else @if /I "%_BUILDARCH%"=="amd64" (
	@set ARCH=amd64
) else @if /I "%_BUILDARCH%"=="IA64" (
	@set ARCH=ia64
) else @goto end

@echo %ARCH%

rem Set MSC_VER
@set RANDFILENAME=%TEMP%\build%RANDOM%
@echo SET MSC_VER=_MSC_VER >%RANDFILENAME%.cpp
@cl /EP %RANDFILENAME%.cpp >%RANDFILENAME%.cmd
@call %RANDFILENAME%.cmd
@del %RANDFILENAME%.cmd
@del %RANDFILENAME%.cpp

set /a VERSION=%MSC_VER%/100

rem Set SRC and DEST
@set SRCPATH=eh_%ARCH%\%VERSION%
@set DESTPATH=..\..\..\obj\%DDKBUILDENV%\%ARCH%\LibCpp\eh_obj

rem Copy obj-files
@if NOT EXIST %DESTPATH% mkdir %DESTPATH%
@del /S /Q %DESTPATH% >nul
@copy %SRCPATH%\*.obj %DESTPATH% >nul

:end