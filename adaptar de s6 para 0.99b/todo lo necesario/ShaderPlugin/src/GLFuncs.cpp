#include "GLFuncs.h"

#include <stdio.h>
#include <stdlib.h>

#define GL_VERSION_STRING            0x1F02
#define GL_SHADING_LANGUAGE_VERSION  0x8B8C

// --- Definicion de los punteros de funcion (declarados extern en GLFuncs.h) ---
void    (__stdcall *glViewport)(GLint,GLint,GLsizei,GLsizei) = 0;
void    (__stdcall *glScissor)(GLint,GLint,GLsizei,GLsizei) = 0;
void    (__stdcall *glClear)(GLbitfield) = 0;
void    (__stdcall *glEnable)(GLenum) = 0;
void    (__stdcall *glDisable)(GLenum) = 0;
void    (__stdcall *glGetIntegerv)(GLenum,GLint*) = 0;
void    (__stdcall *glGetBooleanv)(GLenum,GLboolean*) = 0;
GLboolean (__stdcall *glIsEnabled)(GLenum) = 0;
const GLubyte* (__stdcall *glGetString)(GLenum) = 0;
GLenum  (__stdcall *glGetError)(void) = 0;
void    (__stdcall *glGenTextures)(GLsizei,GLuint*) = 0;
void    (__stdcall *glDeleteTextures)(GLsizei,const GLuint*) = 0;
void    (__stdcall *glBindTexture)(GLenum,GLuint) = 0;
void    (__stdcall *glTexParameteri)(GLenum,GLenum,GLint) = 0;
void    (__stdcall *glTexImage2D)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*) = 0;
void    (__stdcall *glCopyTexSubImage2D)(GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei) = 0;
void    (__stdcall *glActiveTexture)(GLenum) = 0;
void    (__stdcall *glGenBuffers)(GLsizei,GLuint*) = 0;
void    (__stdcall *glDeleteBuffers)(GLsizei,const GLuint*) = 0;
void    (__stdcall *glBindBuffer)(GLenum,GLuint) = 0;
void    (__stdcall *glBufferData)(GLenum,GLsizei,const void*,GLenum) = 0;
void    (__stdcall *glGenVertexArrays)(GLsizei,GLuint*) = 0;
void    (__stdcall *glDeleteVertexArrays)(GLsizei,const GLuint*) = 0;
void    (__stdcall *glBindVertexArray)(GLuint) = 0;
void    (__stdcall *glVertexAttribPointer)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*) = 0;
void    (__stdcall *glEnableVertexAttribArray)(GLuint) = 0;
void    (__stdcall *glDisableVertexAttribArray)(GLuint) = 0;
void    (__stdcall *glDrawArrays)(GLenum,GLint,GLsizei) = 0;
GLuint  (__stdcall *glCreateShader)(GLenum) = 0;
void    (__stdcall *glShaderSource)(GLuint,GLsizei,const GLchar**,const GLint*) = 0;
void    (__stdcall *glCompileShader)(GLuint) = 0;
void    (__stdcall *glGetShaderiv)(GLuint,GLenum,GLint*) = 0;
void    (__stdcall *glGetShaderInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*) = 0;
GLuint  (__stdcall *glCreateProgram)(void) = 0;
void    (__stdcall *glAttachShader)(GLuint,GLuint) = 0;
void    (__stdcall *glLinkProgram)(GLuint) = 0;
void    (__stdcall *glGetProgramiv)(GLuint,GLenum,GLint*) = 0;
void    (__stdcall *glGetProgramInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*) = 0;
void    (__stdcall *glUseProgram)(GLuint) = 0;
void    (__stdcall *glDeleteShader)(GLuint) = 0;
void    (__stdcall *glDeleteProgram)(GLuint) = 0;
GLint   (__stdcall *glGetUniformLocation)(GLuint,const GLchar*) = 0;
void    (__stdcall *glUniform1f)(GLint,GLfloat) = 0;
void    (__stdcall *glUniform1i)(GLint,GLint) = 0;
void    (__stdcall *glUniform2f)(GLint,GLfloat,GLfloat) = 0;
void    (__stdcall *glUniform3f)(GLint,GLfloat,GLfloat,GLfloat) = 0;
void    (__stdcall *glUniform4f)(GLint,GLfloat,GLfloat,GLfloat,GLfloat) = 0;
GLint   (__stdcall *glGetAttribLocation)(GLuint,const GLchar*) = 0;
void    (__stdcall *glFlush)(void) = 0;

// wgl: cargados dinamicamente para no depender de opengl32.lib ni de los
// typedefs de wingdi.h del SDK (que chocarian con nuestras declaraciones)
static HGLRC (__stdcall *s_wglGetCurrentContext)(void) = 0;
static PROC  (__stdcall *s_wglGetProcAddress)(LPCSTR) = 0;

static PROC LoadGLProc(const char* name)
{
	// 1) Intentar la exportacion directa de opengl32.dll (funciones GL 1.1).
	HMODULE gl = GetModuleHandleA("opengl32.dll");

	if(gl != 0)
	{
		PROC p = GetProcAddress(gl,name);

		if(p != 0)
		{
			return p;
		}
	}

	// 2) Funciones 1.2+: wglGetProcAddress (requiere contexto actual).
	if(s_wglGetProcAddress != 0)
	{
		PROC p = s_wglGetProcAddress(name);

		if(p != 0 && (p != (PROC)1) && (p != (PROC)2) && (p != (PROC)3) && (p != (PROC)-1))
		{
			return p;
		}
	}

	return 0;
}

#define LOADF(ret,name,params) \
	do { name = (ret(__stdcall*)params)LoadGLProc(#name); if((name) == 0) return false; } while(0)

bool GLHasCurrentContext()
{
	return (s_wglGetCurrentContext != 0 && s_wglGetCurrentContext() != 0);
}

bool LoadGLFunctions()
{
	// wgl (opengl32.dll siempre cargado en procesos con contexto GL)
	HMODULE gl = GetModuleHandleA("opengl32.dll");

	if(gl != 0)
	{
		s_wglGetCurrentContext = (HGLRC(__stdcall*)(void))GetProcAddress(gl,"wglGetCurrentContext");
		s_wglGetProcAddress    = (PROC (__stdcall*)(LPCSTR))GetProcAddress(gl,"wglGetProcAddress");
	}

	if(s_wglGetCurrentContext == 0 || s_wglGetCurrentContext() == 0)
	{
		return false; // no hay contexto GL actual: la pasada se cargara en el hook
	}

	LOADF(void,glViewport,(GLint,GLint,GLsizei,GLsizei));
	LOADF(void,glScissor,(GLint,GLint,GLsizei,GLsizei));
	LOADF(void,glClear,(GLbitfield));
	LOADF(void,glEnable,(GLenum));
	LOADF(void,glDisable,(GLenum));
	LOADF(void,glGetIntegerv,(GLenum,GLint*));
	LOADF(void,glGetBooleanv,(GLenum,GLboolean*));
	LOADF(GLboolean,glIsEnabled,(GLenum));
	LOADF(const GLubyte*,glGetString,(GLenum));
	LOADF(GLenum,glGetError,(void));
	LOADF(void,glGenTextures,(GLsizei,GLuint*));
	LOADF(void,glDeleteTextures,(GLsizei,const GLuint*));
	LOADF(void,glBindTexture,(GLenum,GLuint));
	LOADF(void,glTexParameteri,(GLenum,GLenum,GLint));
	LOADF(void,glTexImage2D,(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*));
	LOADF(void,glCopyTexSubImage2D,(GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei));
	LOADF(void,glActiveTexture,(GLenum));
	LOADF(void,glGenBuffers,(GLsizei,GLuint*));
	LOADF(void,glDeleteBuffers,(GLsizei,const GLuint*));
	LOADF(void,glBindBuffer,(GLenum,GLuint));
	LOADF(void,glBufferData,(GLenum,GLsizei,const void*,GLenum));
	LOADF(void,glGenVertexArrays,(GLsizei,GLuint*));
	LOADF(void,glDeleteVertexArrays,(GLsizei,const GLuint*));
	LOADF(void,glBindVertexArray,(GLuint));
	LOADF(void,glVertexAttribPointer,(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*));
	LOADF(void,glEnableVertexAttribArray,(GLuint));
	LOADF(void,glDisableVertexAttribArray,(GLuint));
	LOADF(void,glDrawArrays,(GLenum,GLint,GLsizei));
	LOADF(GLuint,glCreateShader,(GLenum));
	LOADF(void,glShaderSource,(GLuint,GLsizei,const GLchar**,const GLint*));
	LOADF(void,glCompileShader,(GLuint));
	LOADF(void,glGetShaderiv,(GLuint,GLenum,GLint*));
	LOADF(void,glGetShaderInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*));
	LOADF(GLuint,glCreateProgram,(void));
	LOADF(void,glAttachShader,(GLuint,GLuint));
	LOADF(void,glLinkProgram,(GLuint));
	LOADF(void,glGetProgramiv,(GLuint,GLenum,GLint*));
	LOADF(void,glGetProgramInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*));
	LOADF(void,glUseProgram,(GLuint));
	LOADF(void,glDeleteShader,(GLuint));
	LOADF(void,glDeleteProgram,(GLuint));
	LOADF(GLint,glGetUniformLocation,(GLuint,const GLchar*));
	LOADF(void,glUniform1f,(GLint,GLfloat));
	LOADF(void,glUniform1i,(GLint,GLint));
	LOADF(void,glUniform2f,(GLint,GLfloat,GLfloat));
	LOADF(void,glUniform3f,(GLint,GLfloat,GLfloat,GLfloat));
	LOADF(void,glUniform4f,(GLint,GLfloat,GLfloat,GLfloat,GLfloat));
	LOADF(GLint,glGetAttribLocation,(GLuint,const GLchar*));
	LOADF(void,glFlush,(void));

	return true;
}

#undef LOADF

const char* GetGLSLVersionString()
{
	if(glGetString == 0)
	{
		return "";
	}

	const GLubyte* s = glGetString(GL_SHADING_LANGUAGE_VERSION);

	return (s != 0) ? (const char*)s : "";
}

bool GLSLSupports330()
{
	const char* s = GetGLSLVersionString();

	if(s == 0 || s[0] == 0)
	{
		return false;
	}

	float v = (float)atof(s);

	return (v >= 3.30f);
}
