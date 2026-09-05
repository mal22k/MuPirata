@echo off
rem ============================================================
rem  Compila glsl_check.exe (validador de shaders con GL real).
rem  Uso: build_glsl_check.bat  &&  glsl_check.exe
rem ============================================================
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" >nul
if errorlevel 1 goto :error

cl /nologo /W3 /I..\src /EHsc glsl_check.cpp ..\src\GLFuncs.cpp /link user32.lib gdi32.lib /SUBSYSTEM:CONSOLE /OUT:glsl_check.exe
if errorlevel 1 goto :error

echo.
echo glsl_check.exe compilado. Ejecuta: glsl_check.exe
goto :eof

:error
echo ERROR compilando glsl_check
