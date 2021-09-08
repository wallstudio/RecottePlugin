@echo off

set ZIP_FILENAME="recotte_plugins_foundation.zip"
set ZIP_WOKING_DIR=".pack\RecottePlugin"


@REM Clan
del %ZIP_FILENAME%
rd /S /Q %ZIP_WOKING_DIR%


@REM Pack
mkdir %ZIP_WOKING_DIR%
copy README.md %ZIP_WOKING_DIR%\README.md
copy LICENSE %ZIP_WOKING_DIR%\LICENSE
copy install.bat %ZIP_WOKING_DIR%\install.bat
copy x64\Release\*.dll %ZIP_WOKING_DIR%\
copy x64\Release\*.exe %ZIP_WOKING_DIR%\

@REM GitHubActionsではArtifactにZip圧縮も含まれる
@REM PowerShell.exe -Command Compress-Archive -Path %ZIP_WOKING_DIR% -DestinationPath %ZIP_FILENAME%
@REM rd /S /Q %ZIP_WOKING_DIR%
