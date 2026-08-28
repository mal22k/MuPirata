#include <windows.h>
#include <string>
#include <fstream>

const std::string TOKEN = "Updater";

extern "C" __declspec(dllexport) void EntryProc()
{
    std::string cmdLine = GetCommandLineA();

    if (cmdLine.find(TOKEN) == std::string::npos)
    {
        // 1. Obtener el directorio donde está main.exe
        char mainPath[MAX_PATH];
        GetModuleFileNameA(NULL, mainPath, MAX_PATH);
        std::string dir(mainPath);
        size_t slashPos = dir.find_last_of("\\/");
        if (slashPos != std::string::npos)
            dir = dir.substr(0, slashPos); // solo la carpeta

        // 2. Obtener el nombre del launcher (por defecto o desde /launcher:)
        std::string launcherExe = "LauncherWeb.exe";
        size_t argPos = cmdLine.find("/launcher:");
        if (argPos != std::string::npos)
        {
            size_t start = argPos + 10;
            size_t end = cmdLine.find(' ', start);
            if (end == std::string::npos)
                end = cmdLine.length();
            launcherExe = cmdLine.substr(start, end - start);
        }

        // 3. Ruta completa
        std::string fullPath = dir + "\\" + launcherExe;

        // 4. Mostrar mensaje de error
        MessageBoxA(NULL, "Please start Game from the Launcher!", "Error", MB_OK | MB_ICONERROR);

        // 5. Intentar abrir el launcher (solo si el archivo existe)
        std::ifstream testFile(fullPath.c_str());
        if (testFile.good())
        {
            testFile.close();
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi = { 0 };
            CreateProcessA(
                fullPath.c_str(),     // aplicación
                NULL,                 // línea de comandos
                NULL,                 // atributos de seguridad
                NULL,                 // atributos de seguridad del hilo
                FALSE,                // heredar manejadores
                0,                    // banderas
                NULL,                 // entorno
                dir.c_str(),          // directorio actual
                &si,
                &pi);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else
        {
            testFile.close();
        }

        // 6. Cerrar el juego
        ExitProcess(0);
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
        EntryProc();
    }
    return TRUE;
}