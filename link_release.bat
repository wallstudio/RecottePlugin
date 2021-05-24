set RECOTTE="C:\Program Files\RecotteStudio"
set RECOTTE=%RECOTTE:"=%

set CURRENT_DIR=%~dp0
openfiles > nul
if errorlevel 1 (
    PowerShell.exe -Command Start-Process \"%~f0\" -ArgumentList %CURRENT_DIR% -Verb runas
    exit
)
cd %RECOTTE%
set PROJECT=%1
set PROJECT=%PROJECT:"=%

cmd /c del /q "d3d11.dll"
cmd /c rmdir "Plugins"
cmd /c mklink "d3d11.dll" "%PROJECT%\x64\Release\RecottePluginFoundation.dll"
cmd /c mklink /d "Plugins" "%PROJECT%\x64\Release"
pause