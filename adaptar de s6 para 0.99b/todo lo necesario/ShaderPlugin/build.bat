@echo off
rem ============================================================
rem  Compila Shader.dll (VS2010, toolset v100).
rem  Necesita: Visual Studio 2010 (o superior, ajustando la ruta).
rem  Uso: build.bat
rem  Resultado: ..\salidas\Shader.dll  (directorio UNICO de salidas)
rem  NOTA: el copiado al cliente (Package_SSeMU_S6_ENG) lo hace el
rem  usuario manualmente. Este script NO copia nada.
rem ============================================================
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" >nul
if errorlevel 1 goto :error

msbuild ShaderPlugin.vcxproj /p:Configuration=Release /p:Platform=Win32 /v:minimal
if errorlevel 1 goto :error

echo.
echo Shader.dll compilada en ..\salidas\Shader.dll
echo Recuerda copiarla manualmente al cliente y regenerar ServerInfo.sse.
goto :eof

:error
echo ERROR compilando Shader.dll
