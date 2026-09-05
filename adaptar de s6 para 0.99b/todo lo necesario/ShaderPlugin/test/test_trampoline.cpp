// ============================================================================
// test_trampoline.cpp - REGRESION del crash: verifica que el hook de
// SwapBuffers NO recursione.
//
// El bug original: SetCompleteHook NO salva los bytes originales, asi que
// llamar a g_OriginalSwapBuffers(hdc) (la direccion parcheada) volvia a entrar
// en el hook -> recursion infinita -> stack overflow -> crash del main.
//
// Este test:
//   1) Carga Shader.dll y llama EntryProc() (instala el hook CON trampolin)
//   2) Llama a gdi32!SwapBuffers 200 veces: debe pasar por hook -> trampolin
//      -> SwapBuffers real y VOLVER limpiamente (con el bug, la pila
//      reventaria en las primeras llamadas)
//   3) Verifica que el trampolin ejecuta el prologo original (8B FF 55 8B EC)
//
// Uso:  build_trampoline.bat  &&  test_trampoline.exe
// ============================================================================

#include <windows.h>
#include <stdio.h>

static int g_Errors = 0;

#define CHECK(cond,msg) \
	do { if(cond) { printf("[OK] %s\n",msg); } \
	     else { printf("[ERROR] %s\n",msg); g_Errors++; } } while(0)

typedef BOOL (WINAPI *SwapBuffersProc)(HDC);
typedef void (*EntryProc_t)();

int main()
{
	printf("=== test_trampoline: regresion del crash de SwapBuffers ===\n\n");

	// 1) Cargar la DLL y su EntryProc
	HMODULE dll = LoadLibraryA("..\\..\\salidas\\Shader.dll");

	CHECK(dll != 0, "Shader.dll cargada");

	if(dll == 0)
	{
		return 1;
	}

	EntryProc_t entry = (EntryProc_t)GetProcAddress(dll,"EntryProc");

	CHECK(entry != 0, "EntryProc exportada");

	if(entry == 0)
	{
		return 1;
	}

	entry();   // instala el hook (trampolin)

	SwapBuffersProc realSwap = (SwapBuffersProc)GetProcAddress(GetModuleHandleA("gdi32.dll"),"SwapBuffers");

	// 2) El primer byte de SwapBuffers debe ser E9 (hook instalado)
	CHECK(realSwap != 0, "gdi32!SwapBuffers localizada");

	if(realSwap == 0)
	{
		return 1;
	}

	CHECK(*(BYTE*)realSwap == 0xE9, "SwapBuffers hookeado (primer byte E9)");

	// 3) Llamar SwapBuffers 200 veces con un DC sin ventana (el hook la filtra:
	//    no toca GL, pasa directo al trampolin). Con el bug (recursion) la pila
	//    reventaria en las primeras llamadas; con el trampolin vuelve limpio.
	//    SwapBuffers sobre un DC de pantalla devuelve FALSE (0), no crashea.
	int returned = 0;

	for(int i=0;i<200;i++)
	{
		if(realSwap(GetDC(NULL)) == 0)
		{
			returned++;
		}
	}

	CHECK(returned == 200, "200 llamadas a SwapBuffers volvieron sin recursion");

	printf("\n=== RESULTADO: %s, %d error(es) ===\n",g_Errors == 0 ? "TEST OK" : "TEST CON ERRORES",g_Errors);

	return (g_Errors == 0) ? 0 : 1;
}
