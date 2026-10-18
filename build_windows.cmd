@echo off
setlocal EnableExtensions
rem Usage: build_windows.cmd x86^|x64^|arm64
set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=x64"
if /I "%ARCH%"=="x86" (
  set "MACHINE=X86"
) else if /I "%ARCH%"=="x64" (
  set "MACHINE=X64"
) else if /I "%ARCH%"=="arm64" (
  set "MACHINE=ARM64"
) else (
  echo Unsupported architecture: %ARCH%
  exit /b 1
)
where cl.exe >nul 2>nul
if errorlevel 1 (
  echo Visual Studio C++ Build Tools must be installed.
  exit /b 1
)
if not exist build mkdir build
cl /nologo /std:c++17 /O2 /Oi- /Gs1048576 /GS- /GR- /EHsc /utf-8 /c /Fo:build\main.obj src\main.cpp
if errorlevel 1 exit /b 1
link /nologo /machine:%MACHINE% /subsystem:windows /entry:WinMainCRTStartup /nodefaultlib /manifest:embed /manifestuac:no /manifestinput:app.manifest /out:RDPCredManager.exe build\main.obj kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib comctl32.lib uxtheme.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /O2 /EHsc /utf-8 /Fe:build\manifest_verify.exe tests\manifest_verify.cpp
if errorlevel 1 exit /b 1
rem Windows ARM64 tests execute natively in the GitHub ARM64 job.
if /I "%ARCH%"=="arm64" exit /b 0
build\manifest_verify.exe RDPCredManager.exe %ARCH%
exit /b %errorlevel%
