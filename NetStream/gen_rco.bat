@echo off
SET PLUGIN=netstream
SET COMPILER=%SCE_PSP2_SDK_DIR%/host_tools/build/rco/bin/acdc.exe
SET TMP=RES_RCO/RES_RCO_TMP

@RD /S /Q "%TMP%"
mkdir "%TMP%"

for %%f in (RES_RCO/locale/*.xml) do (

"%COMPILER%" -c -i "RES_RCO/locale/%%f" -s "%SCE_PSP2_SDK_DIR%/host_tools/build/rco/def/rcs.cxmldef" -o "%TMP%/%%f.rcs"

)

"%COMPILER%" -c -i "RES_RCO/%PLUGIN%_plugin.xml" -s "%SCE_PSP2_SDK_DIR%/host_tools/build/rco/def/rco.cxmldef" -o "CONTENTS/%PLUGIN%_plugin.rco" -r "CONTENTS/%PLUGIN%_plugin.rcd"