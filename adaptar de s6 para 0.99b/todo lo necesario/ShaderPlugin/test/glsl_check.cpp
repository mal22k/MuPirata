// ============================================================================
// glsl_check.cpp - Verifica que post.vs / post.fs compilen en la GPU de ESTA
// maquina. Crea una ventana oculta con contexto OpenGL (el mismo mecanismo que
// usara el cliente), carga las funciones GL y compila/enlaza los shaders.
//
// Uso:  build_glsl_check.bat  &&  glsl_check.exe
// Exit: 0 = shaders OK, 1 = error (muestra el log del compilador GLSL)
// ============================================================================

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/GLFuncs.h"

// --- wgl (opengl32.dll) ---
typedef HGLRC (WINAPI*WGLCreateContext)(HDC);
typedef BOOL  (WINAPI*WGLMakeCurrent)(HDC,HGLRC);
typedef BOOL  (WINAPI*WGLDeleteContext)(HGLRC);

static WGLCreateContext p_wglCreateContext = 0;
static WGLMakeCurrent   p_wglMakeCurrent = 0;
static WGLDeleteContext p_wglDeleteContext = 0;

static const char* g_vsPath = "../shaders/post.vs";
static const char* g_fsPath = "../shaders/post.fs";

// ---------------------------------------------------------------------------
static char* ReadFile(const char* path)
{
	FILE* f = 0;
	fopen_s(&f,path,"rb");

	if(f == 0)
	{
		return 0;
	}

	fseek(f,0,SEEK_END);
	long size = ftell(f);
	fseek(f,0,SEEK_SET);

	char* data = (char*)malloc(size+1);

	if(data != 0)
	{
		fread(data,1,size,f);
		data[size] = 0;
	}

	fclose(f);
	return data;
}

static GLuint CompileShader(GLenum type,const char* source,const char* tag,int& ok)
{
	ok = 0;

	GLuint shader = glCreateShader(type);
	const GLchar* src = source;

	glShaderSource(shader,1,&src,0);
	glCompileShader(shader);

	GLint status = 0;
	glGetShaderiv(shader,GL_COMPILE_STATUS,&status);

	if(status == 0)
	{
		GLchar log[4096] = {0};
		GLsizei len = 0;
		glGetShaderInfoLog(shader,sizeof(log),&len,log);
		printf("[ERROR] %s no compila:\n%s\n",tag,log);
		glDeleteShader(shader);
		return 0;
	}

	printf("[OK] %s compila.\n",tag);
	ok = 1;
	return shader;
}

// ---------------------------------------------------------------------------
static bool CreateContext(HWND wnd)
{
	HDC hdc = GetDC(wnd);

	if(hdc == 0)
	{
		printf("[ERROR] GetDC fallo\n");
		return false;
	}

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd,0,sizeof(pfd));

	pfd.nSize        = sizeof(pfd);
	pfd.nVersion     = 1;
	pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType   = PFD_TYPE_RGBA;
	pfd.cColorBits   = 32;
	pfd.cDepthBits   = 24;
	pfd.iLayerType   = PFD_MAIN_PLANE;

	int pf = ChoosePixelFormat(hdc,&pfd);

	if(pf == 0 || SetPixelFormat(hdc,pf,&pfd) == FALSE)
	{
		printf("[ERROR] SetPixelFormat fallo (pf=%d)\n",pf);
		ReleaseDC(wnd,hdc);
		return false;
	}

	HGLRC ctx = p_wglCreateContext(hdc);

	if(ctx == 0)
	{
		printf("[ERROR] wglCreateContext fallo\n");
		ReleaseDC(wnd,hdc);
		return false;
	}

	if(p_wglMakeCurrent(hdc,ctx) == FALSE)
	{
		printf("[ERROR] wglMakeCurrent fallo\n");
		p_wglDeleteContext(ctx);
		ReleaseDC(wnd,hdc);
		return false;
	}

	printf("[INFO] Contexto OpenGL creado OK\n");
	ReleaseDC(wnd,hdc);
	return true;
}

// ---------------------------------------------------------------------------
int main()
{
	printf("=== glsl_check: validador de shaders (GLSL 330) ===\n");

	// 1) Archivos de shader
	char* vs = ReadFile(g_vsPath);
	char* fs = ReadFile(g_fsPath);

	if(vs == 0 || fs == 0)
	{
		printf("[ERROR] No se encontraron los shaders: %s / %s\n",g_vsPath,g_fsPath);
		printf("        Ejecuta desde la carpeta ShaderPlugin\\test\n");
		return 1;
	}

	printf("[OK] Shaders leidos: %s (%d bytes), %s (%d bytes)\n",g_vsPath,(int)strlen(vs),g_fsPath,(int)strlen(fs));

	// 2) Ventana oculta + contexto GL
	HMODULE gl = GetModuleHandleA("opengl32.dll");

	if(gl == 0)
	{
		gl = LoadLibraryA("opengl32.dll");
	}

	p_wglCreateContext = (WGLCreateContext)GetProcAddress(gl,"wglCreateContext");
	p_wglMakeCurrent   = (WGLMakeCurrent)GetProcAddress(gl,"wglMakeCurrent");
	p_wglDeleteContext = (WGLDeleteContext)GetProcAddress(gl,"wglDeleteContext");

	WNDCLASSA wc;
	memset(&wc,0,sizeof(wc));
	wc.lpfnWndProc = DefWindowProcA;
	wc.hInstance   = GetModuleHandleA(0);
	wc.lpszClassName = "GLSL_CHECK_WND";
	RegisterClassA(&wc);

	HWND wnd = CreateWindowExA(0,"GLSL_CHECK_WND","glsl_check",WS_OVERLAPPEDWINDOW,
		0,0,640,480,0,0,wc.hInstance,0);

	if(wnd == 0)
	{
		printf("[ERROR] CreateWindow fallo\n");
		return 1;
	}

	bool contextOK = CreateContext(wnd);

	if(contextOK == false)
	{
		return 1;
	}

	// 3) Cargar funciones GL
	if(LoadGLFunctions() == false)
	{
		printf("[ERROR] No se pudieron cargar las funciones GL\n");
		return 1;
	}

	printf("[INFO] GLSL: %s\n",GetGLSLVersionString());

	if(GLSLSupports330() == false)
	{
		printf("[ERROR] GLSL < 3.30: los shaders NO compilaran en esta GPU.\n");
		return 1;
	}

	// 4) Compilar + enlazar
	int okVS = 0, okFS = 0;

	GLuint vsShader = CompileShader(GL_VERTEX_SHADER,vs,"post.vs",okVS);
	GLuint fsShader = CompileShader(GL_FRAGMENT_SHADER,fs,"post.fs",okFS);

	free(vs);
	free(fs);

	if(okVS == 0 || okFS == 0)
	{
		return 1;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program,vsShader);
	glAttachShader(program,fsShader);
	glLinkProgram(program);

	GLint status = 0;
	glGetProgramiv(program,GL_LINK_STATUS,&status);

	if(status == 0)
	{
		GLchar log[4096] = {0};
		GLsizei len = 0;
		glGetProgramInfoLog(program,sizeof(log),&len,log);
		printf("[ERROR] Link del programa fallo:\n%s\n",log);
		return 1;
	}

	printf("[OK] Programa enlazado. Uniforms:\n");

	const char* uniforms[] = { "u_Resolution","u_Time","u_PlayerHP","u_WeatherType",
		"u_TransitionProgress","u_WarmTone","screenTexture" };

	for(int n=0;n<7;n++)
	{
		GLint loc = glGetUniformLocation(program,uniforms[n]);
		printf("      %-22s -> %s\n",uniforms[n],(loc >= 0) ? "OK" : "no usado");
	}

	glDeleteProgram(program);

	printf("\n=== RESULTADO: shaders COMPILAN en esta GPU. Todo OK. ===\n");
	return 0;
}
