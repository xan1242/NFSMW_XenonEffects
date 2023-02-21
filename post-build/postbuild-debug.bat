@echo off
:: Prepare directories
echo Creating directories...
IF EXIST "Debug\Debug-Pack" RMDIR /S /Q "Debug\Debug-Pack"
MKDIR "Debug\Debug-Pack"
MKDIR "Debug\Debug-Pack\GLOBAL"
MKDIR "Debug\Debug-Pack\scripts"
:: Summon the binary
echo Copying the binary
COPY /Y "Debug\NFSMW_XenonEffects.asi" "Debug\Debug-Pack\scripts"
:: Summon text files
echo Summoning text files
COPY /Y "NFSMW_XenonEffects.ini" "Debug\Debug-Pack\scripts"
:: Build the TPK -- YOU NEED TO HAVE XNFSTPKTool in your PATH!!!
echo Building TPK with XNFSTPKTool
xnfstpktool -w2 "Texture\XenonEffects.ini" "Debug\Debug-Pack\GLOBAL\XenonEffects.tpk"

echo Post build done!
