@echo off
rem parameters: 1 = APP name,  2 = APP UID,  3 = platform (S60, ...), 4 = def file (optional)
rem environment: OBJS = object files,  L = path to intermediate object files

if %3==S60 set LL=%IM_SYMB_S60_SDK%\Release\armi\urel
set EPOCLIBS=%LL%\sysagt.lib %LL%\EDLL.LIB %LL%\EDLLSTUB.LIB %LL%\euser.lib %LL%\apparc.lib %LL%\cone.lib %LL%\eikcore.lib %LL%\ws32.lib %LL%\bitgdi.lib %LL%\fbscli.lib %LL%\efsrv.lib %LL%\bafl.lib %LL%\gdi.lib %LL%\hal.lib %LL%\msgs.lib %LL%\insock.lib %LL%\ezlib.lib %LL%\commdb.lib %LL%\plpvariant.lib %LL%\apgrfx.lib %LL%\sdpagent.LIB %LL%\sdpdatabase.LIB %LL%\btextnotifiers.lib %LL%\bluetooth.lib %LL%\MediaClientAudio.lib %LL%\cntmodel.lib %LL%\EikCoCtl.lib %LL%\fepbase.lib %LL%\irobex.lib %LL%\esock.lib %LL%\MediaClientAudioStream.lib %LL%\SecureSocket.lib %LL%\CharConv.lib %LL%\etel.lib %LL%\EStor.lib %LL%\EText.lib %LL%\Estlib.lib
if %3==S60 set EPOCLIBS=%EPOCLIBS% %LL%\Avkon.lib %LL%\SendUi.lib


if "%4"=="" ( set DEF_FILE=..\..\Tools\Symbian\7.0\SymbianApp.def ) else ( set DEF_FILE=%4 )

set path=..\..\Tools\Symbian\7.0\gcc\bin;..\..\Tools\Symbian\7.0\Tools

set DLLTARGS=-m arm_interwork --def "%DEF_FILE%" --dllname MyGame[%2].app --output-exp %L%tmp.exp
set LDARGS= -s -e _E32Dll --image-base 0 %L%tmp.exp --dll -o %L%%1.app %OBJS% "..\..\lib\%3_Release\SymbLib.lib" "..\..\lib\%3_Release\ImgLib.lib" %EPOCLIBS%

rem// generate 1st pass .exp file
dlltool %DLLTARGS%
if errorlevel 1 goto fail1

rem// link 1st pass
ld %LDARGS% --base-file %L%tmp.bas
if errorlevel 1 goto fail2

rem// generate 2nd pass .exp file
dlltool %DLLTARGS% --base-file %L%tmp.bas
if errorlevel 1 goto fail1

rem// link 2nd pass
ld %LDARGS% -Map %L%%1.map
if errorlevel 1 goto fail2

rem copy %L%%1.app %L%%1.appx
if NOT "%CODE_PATCH%" == "" %CODE_PATCH% %CODE_PATCH_PARAMS% %L%%1.app

rem// process by petran
rem petran %L%%1.app %L%%1.app -nocall -uid1 0x10000079 -uid2 0x100039ce -uid3 0x%2 >nul
rem copy %L%%1.app %L%raw.app >nul
"..\..\Tools\Symbian\7.0\ECompXLTran" %L%%1.app %L%%1.app "..\..\Tools\Symbian\7.0\ECompXL\Stub_%3.app" -nocall -uid1 0x10000079 -uid2 0x100039ce -uid3 0x%2 >nul
if errorlevel 1 goto fail3
del %L%tmp.bas %L%tmp.exp

goto end

:fail1
echo Can't run: dlltool %DLLTARGS%
goto :end

:fail2
echo Can't run: ld %LDARGS%
goto :end

:fail3
echo Can't run EXompXlTran
goto :end

:fail4
@echo Failed!

:end

