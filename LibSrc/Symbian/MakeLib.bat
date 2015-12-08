rem Params:  %1 = ObjDir, %2 = lib-name [, %3 = SYMB_9_1 ]

if exist "%2" del "%2"
set OBJS=%1\About.o %1\Base64.o %1\Bits.o %1\C_file.o %1\C_file_base.o %1\Cstr.o %1\C_math.o %1\C_vector.o %1\Config.o
set OBJS=%OBJS% %1\Directory.o %1\DynamicArmCode.o %1\EncryptedCode.o %1\FatalError.o %1\Fixed.o %1\FormattedMessage.o %1\GlobalData.o %1\Graphics.o
set OBJS=%OBJS% %1\IGraph.o %1\Image.o %1\InfoMessage.o %1\Lang.o %1\LogMan.o %1\Md5.o %1\Memory.o %1\Mode.o %1\MultiQuestion.o %1\Network.o
set OBJS=%OBJS% %1\Panic.o %1\Profiler.o %1\Question.o %1\QuickSort.o %1\RndGen.o
set OBJS=%OBJS% %1\SerialNum.o %1\SerialNumSymbian.o %1\Skins.o %1\SndPlayer.o %1\Socket.o %1\SocketBase.o %1\SoundBase.o %1\SoundMixer.o %1\SymbianAppStart.o %1\System.o
set OBJS=%OBJS% %1\TextEdit.o %1\TextEditSymbian.o %1\TextEntry.o %1\TextUtils.o %1\Thread.o %1\ThreadSymbian.o %1\TickCount.o %1\TimeDate.o %1\TimeDateSymbian.o %1\TinyEncrypt.o %1\UserInterface.o
set OBJS=%OBJS% %1\Vibration.o %1\Xml.o %1\ZipCreate.o %1\ZipRawFile.o %1\ZipRead.o

if "%3" == "SYMB_9_1" (
..\..\Tools\Symbian\9.1\gcc\bin\arm-none-symbianelf-ar.exe -c -r %2 %OBJS%
) else (
rem %1\Helper.o
..\..\Tools\Symbian\7.0\gcc\bin\ar.exe -r %2 %OBJS% %1\E32Dll.o
)
if errorlevel 1 del "%2"
