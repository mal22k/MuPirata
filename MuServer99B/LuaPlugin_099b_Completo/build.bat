@echo off
rem ============================================================
rem  Compila Lua.dll (VS2010, toolset v100).
rem  Necesita: Visual Studio 2010 (o superior, ajustando la ruta).
rem  Uso: build.bat
rem  Resultado: ..\salidas\Lua.dll  (directorio UNICO de salidas)
rem  NOTA: el copiado al cliente (Package_SSeMU_S6_ENG) lo hace el
rem  usuario manualmente. Este script NO copia nada.
rem ============================================================
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" >nul
if errorlevel 1 goto :error

msbuild LuaPlugin.vcxproj /p:Configuration=Release /p:Platform=Win32 /v:minimal
if errorlevel 1 goto :error

echo.
echo Lua.dll compilada en ..\salidas\Lua.dll
echo Recuerda copiarla manualmente al cliente.
goto :eof

:error
echo ERROR compilando Lua.dll
