@echo off
:: Prepare directories
echo Creating directories...
IF EXIST "Release\Release-Pack" RMDIR /S /Q "Release\Release-Pack"
MKDIR "Release\Release-Pack"
MKDIR "Release\Release-Pack\GLOBAL"
MKDIR "Release\Release-Pack\scripts"
:: Summon the binary
echo Copying the binary
COPY /Y "Release\NFSMW_XenonEffects.asi" "Release\Release-Pack\scripts"
:: Summon text files
echo Summoning text files
COPY /Y "NFSMW_XenonEffects.ini" "Release\Release-Pack\scripts"
:: Generate the TPK ini
echo Generating the TPK ini
CALL "Texture\TpkIniGen.bat" "Texture" "Texture\XenonEffects.ini"
:: Build the TPK -- YOU NEED TO HAVE XNFSTPKTool in your PATH!!!
echo Building TPK with XNFSTPKTool
xnfstpktool -w2 "Texture\XenonEffects.ini" "Release\Release-Pack\GLOBAL\XenonEffects.tpk"

echo Post build done!
