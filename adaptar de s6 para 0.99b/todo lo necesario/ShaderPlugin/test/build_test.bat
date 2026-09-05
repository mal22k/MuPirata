@echo off
rem ============================================================
rem  Compila el test harness de logica del ShaderPlugin.
rem  No necesita GPU ni el cliente MU: solo VS2010.
rem  Uso: build_test.bat  &&  test_shader.exe
rem ============================================================
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" >nul
if errorlevel 1 goto :error

cl /nologo /W3 /I..\src /EHsc test_shader.cpp ..\src\PostProcess.cpp ..\src\GLFuncs.cpp /link user32.lib /SUBSYSTEM:CONSOLE /OUT:test_shader.exe
if errorlevel 1 goto :error

echo.
echo Test compilado. Ejecuta: test_shader.exe
goto :eof

:error
echo ERROR compilando el test.
