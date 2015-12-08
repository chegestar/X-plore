set L=_build\Explorer\%1_Release\
call ..\..\Tools\Symbian\7.0\Link.bat X-plore 20009598 %1
if errorlevel 1 goto fail

rem// create crc file for app (use app's ID as seed)
..\..\Tools\MakeCRC.exe %L%X-plore.app 0x20009598
copy %L%X-plore.app /B + %L%X-plore.app.crc %L%X-plore.app >nul
del %L%X-plore.app.crc

call :make_shops %1

goto :eof

:fail
@echo Failed!
goto :eof

//----------------------------
:make_shops

copy ..\Explorer\Data.dta %L% >nul

rem Pack stuff into DTA
..\..\Tools\ZipMaker +%L%\Data.dta ^
"Lang { ^
   %LANG_DIR%\ ^
}"

call :make_shop %1 lcg
if errorlevel 1 goto fail1

goto :eof

:fail1
echo Failed PrepareShop
goto :eof
//----------------------------

:make_shop

rem// make sis file
makesis Symbian\Explorer\%1.pkg _build\Explorer\X-plore_%1_%2.sis 
if errorlevel 1 goto fail2

echo SIS Done (%2)
goto :eof

:fail2
makesis Symbian\Explorer\%1.pkg
goto :eof

//----------------------------
