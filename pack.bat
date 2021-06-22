

mkdir recotte_plugins_foundation\Plugins
copy x64\Debug\* recotte_plugins_foundation\Plugins\
copy x64\Debug\RecottePluginFoundation.dll recotte_plugins_foundation\d3d11.dll
copy LICENSE recotte_plugins_foundation\
copy README.md recotte_plugins_foundation\
PowerShell.exe -Command Compress-Archive -Path recotte_plugins_foundation\* -DestinationPath recotte_plugins_foundation.zip

rmdir /s /q recotte_plugins_foundation