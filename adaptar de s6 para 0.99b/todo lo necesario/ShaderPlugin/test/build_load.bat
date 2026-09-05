@echo off
rem ============================================================
rem  Compila load_dll.exe (verifica que Shader.dll carga + EntryProc).
rem  Uso: build_load.bat  &&  load_dll.exe
rem ============================================================
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" >nul
if errorlevel 1 goto :error

cl /nologo /EHsc load_dll.cpp /link /SUBSYSTEM:CONSOLE /OUT:load_dll.exe
if errorlevel 1 goto :error

echo.
echo load_dll.exe compilado. Ejecuta: load_dll.exe
goto :eof

:error
echo ERROR compilando load_dll
