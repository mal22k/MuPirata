@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
cl /nologo /W3 ini_read_test.c /link /SUBSYSTEM:CONSOLE /OUT:ini_read_test.exe
if errorlevel 1 goto :err
ini_read_test.exe
goto :eof
:err
echo ERROR compilando
