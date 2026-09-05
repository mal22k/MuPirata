// ============================================================================
// load_dll.cpp - Verifica que Shader.dll carga en un proceso (sin el cliente)
// y que exporta EntryProc (lo que llama main.dll via GetProcAddress).
//
// Uso:  cl load_dll.cpp && load_dll.exe   (o build_glsl_check.bat, mismo cl)
// Exit: 0 = OK
// ============================================================================

#include <windows.h>
#include <stdio.h>

int main()
{
	printf("=== load_dll: verificando Shader.dll ===\n");

	HMODULE h = LoadLibraryA("..\\..\\salidas\\Shader.dll");

	if(h == 0)
	{
		printf("[ERROR] LoadLibrary fallo, GetLastError=%d\n",GetLastError());
		return 1;
	}

	FARPROC ep = GetProcAddress(h,"EntryProc");

	if(ep == 0)
	{
		printf("[ERROR] La DLL no exporta EntryProc\n");
		FreeLibrary(h);
		return 1;
	}

	printf("[OK] Shader.dll cargada, EntryProc en 0x%p\n",(void*)ep);

	// Dependencias presentes (se resolverian al cargar con el cliente)
	HMODULE gdi = GetModuleHandleA("gdi32.dll");
	printf("[%s] gdi32.dll %s\n",(gdi != 0) ? "OK" : "INFO",(gdi != 0) ? "cargada" : "se carga bajo demanda");

	FreeLibrary(h);
	printf("\n=== RESULTADO: Shader.dll lista para el registro ===\n");
	return 0;
}
