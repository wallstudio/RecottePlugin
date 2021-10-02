@REM レコスタのインストール場所を変更している場合はこれを書き換えてください
set RECOTTE="C:\Program Files\RecotteStudio" 

@echo off
chcp 65001 
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

echo 古いRecottePluginをアンインストールします
cmd /c del /q "d3d11.dll"
cmd /c rmdir /S /Q "Plugins"

echo 新しいRecottePluginをインストールします
cmd /c mklink "d3d11.dll" "%PROJECT%RecottePluginManager.dll"

echo RecottePluginをインストールしました
pause