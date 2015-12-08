@echo off
rem parameters: 1 = APP name,  2 = APP UID,  3 = platform (S60, ...), [4 = capability] [5 = forced_stub_id]
rem environment: OBJS = object files,  L = path to intermediate object files

set CAPS=%4
if "%4"=="" set CAPS=none

set path=..\..\Tools\Symbian\9.1\gcc\bin;..\..\Tools\Symbian\9.1\Tools
if %3==S60 set SDK_PATH=%IM_SYMB_S60_3RD_SDK%

set LL=%SDK_PATH%\Release\armv5\lib
set EPOCLIBS=%LL%\Euser.dso %LL%\drtaeabi.dso %LL%\scppnwdl.dso %LL%\EikCore.dso %LL%\Cone.dso %LL%\Apparc.dso %LL%\Ws32.dso %LL%\Efsrv.dso %LL%\Fbscli.dso %LL%\BitGdi.dso %LL%\Ezlib.dso %LL%\Bafl.dso %LL%\Gdi.dso %LL%\dfpaeabi.dso %LL%\mediaclientaudiostream.dso %LL%\Etel3rdparty.dso %LL%\ECom.dso %LL%\Scdv.dso %LL%\ApMime.dso %LL%\Apgrfx.dso %LL%\hal.dso %LL%\Charconv.dso %LL%\smcm.dso %LL%\msgs.dso %LL%\CntModel.dso %LL%\ESock.dso %LL%\irobex.dso %LL%\eikcoctl.dso %LL%\SdpDataBase.dso %LL%\SdpAgent.dso %LL%\centralrepository.dso %LL%\commdb.dso %LL%\btextnotifiers.dso %LL%\btmanclient.dso %LL%\securesocket.dso %LL%\irda.dso %LL%\InSock.dso %LL%\FepBase.dso %LL%\mediaclientaudio.dso %LL%\bluetooth.dso %LL%\EikCtl.dso %LL%\RemConInterfaceBase.dso %LL%\RemConCoreApi.dso %LL%\mmfdevsound.dso %LL%\mmfserverbaseclasses.dso %LL%\AlarmClient.dso %LL%\AlarmShared.dso %LL%\EikCtl.dso %LL%\estlib.dso %LL%\Ecam.dso %LL%\mediaclientvideo.dso %LL%\CharConv.dso %LL%\ImageConversion.dso %LL%\estor.dso %LL%\calinterimapi.dso %LL%\c32.dso %LL%\Egul.dso %LL%\EText.dso
if %3==S60 set EPOCLIBS=%EPOCLIBS% %LL%\avkon.dso %LL%\hwrmlightclient.dso %LL%\AknsWallpaperUtils.dso %LL%\ApEngine.dso %LL%\FeatDiscovery.dso %LL%\AknSkins.dso %LL%\ProfileEng.dso %LL%\PtiEngine.dso %LL%\SendUi.dso %LL%\commonui.dso %LL%\browserengine.dso
rem %LL%\DevVideo.dso

rem if exist %IM_SYMB_3RD_SHARED%\gcc\lib\gcc\arm-none-symbianelf\3.4.3 (set GCC_VER=3.4.3) ELSE (set GCC_VER=4.2.0)
set GCC_VER=4.4.1
set LL=%SDK_PATH%\Release\armv5\urel
set EPOCLIBS=%EPOCLIBS% %LL%\eexe.lib %LL%\usrt2_2.lib
set EPOCLIBS=%EPOCLIBS% ..\..\Tools\Symbian\9.1\gcc\arm-none-symbianelf\lib\libsupc++.a ..\..\Tools\Symbian\9.1\gcc\lib\gcc\arm-none-symbianelf\%GCC_VER%\libgcc.a

rem// link (create elf.exe)
arm-none-symbianelf-ld -shared -g -Map %L%\%1.map -o %L%\elf.exe %OBJS% --entry Lcg32Main %EPOCLIBS% "..\..\Lib\%3_Release_3RD\SymbLib.lib" "..\..\Lib\S60_Release_3RD\ImgLib.lib"
if errorlevel 1 goto fail2

rem// process by elf2e32 (create e32.exe)
elf2e32 --uid1=0x1000007a --sid=0 --targettype=EXE --uncompressed --elfinput=%L%\elf.exe --output=%L%\e32.exe --libpath=%SDK_PATH%\Release\armv5\lib
if errorlevel 1 goto fail3
rem del %L%\elf.exe

if NOT "%CODE_PATCH%" == "" %CODE_PATCH% %CODE_PATCH_PARAMS% %L%\e32.exe
if errorlevel 1 goto fail4

if "%5"=="*" goto plain_link

rem --------------------------------------------------------

rem //creation of encrypted binary executable

rem// copy stub (elfstub.exe)
rem copy ..\..\Tools\Symbian\9.1\E32ToLcg32\Lcg32LoaderStub.exe %L%\elfstub.exe >nul

rem// create encrypted binary file (lcg32.bin); patch stub with crc's
rem ..\..\Tools\Symbian\9.1\E32ToLcg32\E32toLcg32.exe %L%\e32.exe %L%\lcg32.bin %L%\elfstub.exe
"..\..\Tools\Symbian\9.1\E32ToLcg32\E32toLcg32.exe" %3 %L%\e32.exe %L%\lcg32.bin "..\..\Tools\Symbian\9.1\E32ToLcg32\Lcg32LoaderStub.exe" %L%\StubE32.exe 0x%2 %CAPS% %5
if errorlevel 1 goto fail4
del %L%\e32.exe

rem// convert stub to e32 (StubE32.exe)
rem elf2e32 --uid1=0x1000007a --uid2=0 --uid3=0x%2 --sid=0x%2 --vid=0 --capability=%CAPS% --targettype=EXE --stack=0x5000 --heap=0x100000,0x600000 --elfinput=%L%\ElfStub.exe --output=%L%\StubE32.exe --libpath=%SDK_PATH%\Release\armv5\lib
if errorlevel 1 goto fail3
rem del %L%\elfstub.exe

goto :eof

rem --------------------------------------------------------
rem //plain executable linking

:plain_link
arm-none-symbianelf-ld -shared --entry _E32Startup -u _E32Startup -Ttext 0x8000 -Tdata 0x400000 -o %L%\tmp.exe %OBJS% "..\..\Lib\%3_Release_3RD\SymbLib.lib" %EPOCLIBS%
if errorlevel 1 goto fail2

rem// process by elf2e32
elf2e32 --uid1=0x1000007a --uid2=0 --uid3=0x%2 --sid=0x%2 --vid=0 --capability=%CAPS% --targettype=EXE --stack=0x5000 --heap=0x100000,0x600000 --elfinput=%L%\tmp.exe --output=%L%\%1.exe --linkas=%1{000a0000}[%2].exe --libpath=%SDK_PATH%\Release\armv5\lib
if errorlevel 1 goto fail3
del %L%\tmp.exe

goto :eof

:fail2
echo Can't run: ld
goto :eof


:fail3
echo Can't run: elf2e32
goto :eof

:fail4
echo Can't run: E32ToLcg32
goto :eof
