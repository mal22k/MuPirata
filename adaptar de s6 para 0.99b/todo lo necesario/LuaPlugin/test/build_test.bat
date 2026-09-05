@echo off
rem ============================================================
rem  Compila el test harness standalone del LuaPlugin.
rem  No necesita el cliente MU: solo VS2010 (v100) + lua52.lib.
rem  Si tu VS2010 esta en otra ruta, ajusta la linea del call.
rem  Uso:  build_test.bat   y luego   test_lua.exe
rem ============================================================
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" >nul
cl /nologo /W3 /I..\lua /EHsc test_lua.cpp /link ..\lua\lua52.lib /SUBSYSTEM:CONSOLE /OUT:test_lua.exe
if errorlevel 1 goto :error
echo.
echo Test compilado. Ejecuta: test_lua.exe
goto :eof

:error
echo ERROR compilando el test.
