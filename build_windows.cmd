@echo off
setlocal
where cl.exe >nul 2>nul
if errorlevel 1 (
  echo Visual Studio C++ Build Tools must be installed.
  exit /b 1
)
if not exist build mkdir build
cl /nologo /std:c++17 /O2 /Oi- /GS- /GR- /EHsc /utf-8 /c /Fo:build\main.obj src\main.cpp
if errorlevel 1 exit /b 1
link /nologo /machine:x64 /subsystem:windows /entry:WinMainCRTStartup /nodefaultlib /manifest:embed /manifestuac:no /manifestinput:app.manifest /out:RDPCredManager.exe build\main.obj kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib comctl32.lib uxtheme.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /O2 /EHsc /utf-8 /Fe:build\manifest_verify.exe tests\manifest_verify.cpp
if errorlevel 1 exit /b 1
build\manifest_verify.exe RDPCredManager.exe
exit /b %errorlevel%
