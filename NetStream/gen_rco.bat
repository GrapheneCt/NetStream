@echo off
SET PLUGIN=netstream_plugin
SET PYTHON=%SCE_PSP2_SDK_DIR%/host_tools/build/rco/Python27/python.exe
SET Z=%SCE_PSP2_SDK_DIR%/host_tools/build/rco/bin\zdrop.exe
SET D=%SCE_PSP2_SDK_DIR%/host_tools/build/rco/bin\psp2rcd.exe
SET RCS=%SCE_PSP2_SDK_DIR%/host_tools/build/rco/cxml/appinfo/rcs_compiler.py
SET RCO=%SCE_PSP2_SDK_DIR%/host_tools/build/rco/cxml/appinfo/appinfo_compiler.py
SET TMP=RES_RCO/RES_RCO_TMP

@RD /S /Q "%TMP%"
mkdir "%TMP%"
copy /b "%Z%" "%TMP%"

for %%f in (RES_RCO/locale/*.xml) do (

"%PYTHON%" "%RCS%" -o "%TMP%/%%f.rcs" RES_RCO/locale/%%f
"%TMP%/zdrop.exe" "%TMP%/%%f.rcs"

)


for %%f in (RES_RCO/file/*) do (

copy /b "RES_RCO/file\%%f" "%TMP%"
"%TMP%/zdrop.exe" "%TMP%/%%f"

)

"%PYTHON%" "%RCO%" -o CONTENTS/"%PLUGIN%".rco RES_RCO/"%PLUGIN%".xml
"%D%" -o CONTENTS/"%PLUGIN%".rcd -i RES_RCO/"%PLUGIN%".xml