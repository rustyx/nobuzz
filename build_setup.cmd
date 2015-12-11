@echo off
set WIX_HOME="..\packages\WiX.3.10.1\tools"
cd Setup
mkdir bin\Release
%WIX_HOME%\candle.exe -o bin\Release\ nobuzz32.wxs
%WIX_HOME%\light.exe -o bin\Release\nobuzz_setup.msi bin\Release\nobuzz32.wixobj
%WIX_HOME%\candle.exe -arch x64 -o bin\Release\ nobuzz64.wxs
%WIX_HOME%\light.exe -o bin\Release\nobuzz_setup_64.msi bin\Release\nobuzz64.wixobj
%WIX_HOME%\candle.exe -o bin\Release\ -ext WixBalExtension Bundle.wxs
%WIX_HOME%\light.exe -o bin\Release\nobuzz_setup.exe -ext WixBalExtension -ext WixUtilExtension bin\Release\Bundle.wixobj
cd ..
pause
