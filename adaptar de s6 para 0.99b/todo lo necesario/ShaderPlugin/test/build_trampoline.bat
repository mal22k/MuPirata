@echo off
rem ============================================================
rem  Compila test_trampoline.exe (regresion del crash por recursion
rem  en el hook de SwapBuffers). Necesita la DLL ya compilada en ..\..\salidas.
rem  Uso: build_trampoline.bat  &&  test_trampoline.exe
rem ============================================================
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" >nul
if errorlevel 1 goto :error

cl /nologo /W3 /EHsc test_trampoline.cpp /link user32.lib gdi32.lib /SUBSYSTEM:CONSOLE /OUT:test_trampoline.exe
if errorlevel 1 goto :error

echo.
echo test_trampoline.exe compilado. Ejecuta: test_trampoline.exe
goto :eof

:error
echo ERROR compilando test_trampoline
