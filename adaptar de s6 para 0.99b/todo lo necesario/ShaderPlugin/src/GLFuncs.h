#pragma once

// ============================================================================
// GLFuncs.h - Carga dinamica de funciones OpenGL.
//
// La DLL NO enlaza opengl32.lib ni usa headers de GL: todas las funciones se
// resuelven en tiempo de ejecucion via wglGetProcAddress (funciones 1.2+) con
// fallback a GetProcAddress(opengl32.dll) (funciones 1.1). Asi:
//   - Funciona con cualquier version de OpenGL instalada
//   - Si la GPU no soporta shaders (GLSL < 3.3) simplemente se desactiva
//   - No hay conflictos de headers/lib con el resto del proyecto
// ============================================================================

#include <windows.h>

// --- Constantes GL usadas (sin depender de gl.h) ---
#define GL_TEXTURE_2D          0x0DE1
#define GL_RGBA                0x1908
#define GL_UNSIGNED_BYTE       0x1401
#define GL_NEAREST             0x2600
#define GL_LINEAR              0x2601
#define GL_CLAMP_TO_EDGE       0x812F
#define GL_TEXTURE_MIN_FILTER  0x2801
#define GL_TEXTURE_MAG_FILTER  0x2800
#define GL_TEXTURE_WRAP_S      0x2802
#define GL_TEXTURE_WRAP_T      0x2803
#define GL_VIEWPORT            0x0BA2
#define GL_ARRAY_BUFFER        0x8892
#define GL_STATIC_DRAW         0x88E4
#define GL_FLOAT               0x1406
#define GL_FALSE               0x0000
#define GL_TRIANGLES           0x0004
#define GL_CURRENT_PROGRAM     0x8B8D
#define GL_ACTIVE_TEXTURE      0x84E0
#define GL_TEXTURE0            0x84C0
#define GL_TEXTURE_BINDING_2D  0x8069
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#define GL_BLEND               0x0BE2
#define GL_DEPTH_TEST          0x0B71
#define GL_CULL_FACE           0x0B44
#define GL_SCISSOR_TEST        0x0C11
#define GL_SCISSOR_BOX         0x0C10
#define GL_VERTEX_SHADER       0x8B31
#define GL_FRAGMENT_SHADER     0x8B30
#define GL_COMPILE_STATUS      0x8B81
#define GL_LINK_STATUS         0x8B82
#define GL_INFO_LOG_LENGTH     0x8B84
#define GL_COLOR_BUFFER_BIT    0x00004000

// --- Tipos base (suficientes para los punteros que necesitamos) ---
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned char  GLubyte;
typedef unsigned int   GLbitfield;
typedef int            GLint;
typedef unsigned int   GLuint;
typedef int            GLsizei;
typedef float          GLfloat;
typedef double         GLdouble;
typedef char           GLchar;

// --- Punteros de funcion (cargados en LoadGLFunctions) ---
extern void    (__stdcall *glViewport)(GLint,GLint,GLsizei,GLsizei);
extern void    (__stdcall *glScissor)(GLint,GLint,GLsizei,GLsizei);
extern void    (__stdcall *glClear)(GLbitfield);
extern void    (__stdcall *glEnable)(GLenum);
extern void    (__stdcall *glDisable)(GLenum);
extern void    (__stdcall *glGetIntegerv)(GLenum,GLint*);
extern void    (__stdcall *glGetBooleanv)(GLenum,GLboolean*);
extern GLboolean (__stdcall *glIsEnabled)(GLenum);
extern const GLubyte* (__stdcall *glGetString)(GLenum);
extern GLenum  (__stdcall *glGetError)(void);
extern void    (__stdcall *glGenTextures)(GLsizei,GLuint*);
extern void    (__stdcall *glDeleteTextures)(GLsizei,const GLuint*);
extern void    (__stdcall *glBindTexture)(GLenum,GLuint);
extern void    (__stdcall *glTexParameteri)(GLenum,GLenum,GLint);
extern void    (__stdcall *glTexImage2D)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
extern void    (__stdcall *glCopyTexSubImage2D)(GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
extern void    (__stdcall *glActiveTexture)(GLenum);
extern void    (__stdcall *glGenBuffers)(GLsizei,GLuint*);
extern void    (__stdcall *glDeleteBuffers)(GLsizei,const GLuint*);
extern void    (__stdcall *glBindBuffer)(GLenum,GLuint);
extern void    (__stdcall *glBufferData)(GLenum,GLsizei,const void*,GLenum);
extern void    (__stdcall *glGenVertexArrays)(GLsizei,GLuint*);
extern void    (__stdcall *glDeleteVertexArrays)(GLsizei,const GLuint*);
extern void    (__stdcall *glBindVertexArray)(GLuint);
extern void    (__stdcall *glVertexAttribPointer)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);
extern void    (__stdcall *glEnableVertexAttribArray)(GLuint);
extern void    (__stdcall *glDisableVertexAttribArray)(GLuint);
extern void    (__stdcall *glDrawArrays)(GLenum,GLint,GLsizei);
extern GLuint  (__stdcall *glCreateShader)(GLenum);
extern void    (__stdcall *glShaderSource)(GLuint,GLsizei,const GLchar**,const GLint*);
extern void    (__stdcall *glCompileShader)(GLuint);
extern void    (__stdcall *glGetShaderiv)(GLuint,GLenum,GLint*);
extern void    (__stdcall *glGetShaderInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*);
extern GLuint  (__stdcall *glCreateProgram)(void);
extern void    (__stdcall *glAttachShader)(GLuint,GLuint);
extern void    (__stdcall *glLinkProgram)(GLuint);
extern void    (__stdcall *glGetProgramiv)(GLuint,GLenum,GLint*);
extern void    (__stdcall *glGetProgramInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*);
extern void    (__stdcall *glUseProgram)(GLuint);
extern void    (__stdcall *glDeleteShader)(GLuint);
extern void    (__stdcall *glDeleteProgram)(GLuint);
extern GLint   (__stdcall *glGetUniformLocation)(GLuint,const GLchar*);
extern void    (__stdcall *glUniform1f)(GLint,GLfloat);
extern void    (__stdcall *glUniform1i)(GLint,GLint);
extern void    (__stdcall *glUniform2f)(GLint,GLfloat,GLfloat);
extern void    (__stdcall *glUniform3f)(GLint,GLfloat,GLfloat,GLfloat);
extern void    (__stdcall *glUniform4f)(GLint,GLfloat,GLfloat,GLfloat,GLfloat);
extern GLint   (__stdcall *glGetAttribLocation)(GLuint,const GLchar*);
extern void    (__stdcall *glFlush)(void);

// Devuelve true si pudo resolver TODAS las funciones necesarias.
bool LoadGLFunctions();

// true si el hilo actual tiene un contexto GL actual (wglGetCurrentContext != 0).
// Si es false, NINGUNA llamada GL es segura: hay que esperar al proximo frame.
bool GLHasCurrentContext();

// Devuelve la version de GLSL del contexto actual ("3.30 NVIDIA ..." o "").
const char* GetGLSLVersionString();

// true si la version de GLSL es >= 3.30 (requisito de los shaders).
bool GLSLSupports330();
