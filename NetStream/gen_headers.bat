@echo off
SET PLUGIN=netstream
SET HASHGEN=%SCE_PSP2_SDK_DIR%/host_tools/build/rco/bin/psp2pafhashgen.exe

"%HASHGEN%" -i "RES_RCO\%PLUGIN%_plugin.xml" -o "include\%PLUGIN%_plugin.h"
"%HASHGEN%" -i "RES_RCO\file\%PLUGIN%_settings.xml" -o "include\%PLUGIN%_settings.h"
"%HASHGEN%" -i "RES_RCO\locale\%PLUGIN%_locale_en.xml" -o "include\%PLUGIN%_locale.h"