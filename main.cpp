#include <stdio.h> 

#include <windows.h>
#include <tlhelp32.h>

class Injector
{
public:
	static bool Inject(char *procName, char *dllName)
	{
		// Get the process id from the process name
		DWORD processID = GetTargetThreadIDFromProcName(procName);

		return Inject(processID, dllName);
	}

	static bool Inject(DWORD processID, char *relativeDllName)
	{

		if (processID == 0)
			return false; // tlhelp was unable to find the process name

		HANDLE Proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
		if (Proc == 0)
		{
			// this process id clearly isnt valid or is 0 - bail!
			// this can also happen if we dont have the privileges required to access this process
			printf("OpenProcess() failed: %d", GetLastError());
			return false;
		}

		char DllName[MAX_PATH];

		// get the full path to the dll - this is important as the target processes working directory is probably different !
		GetFullPathNameA(relativeDllName, MAX_PATH, DllName, NULL);

		printf("%s\n", DllName);

		LPVOID LoadLib = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

		// Allocate space in the process for our DLL name
		LPVOID RemoteString = VirtualAllocEx(Proc, NULL, strlen(DllName), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		// Write the string name of our DLL in the memory allocated
		WriteProcessMemory(Proc, RemoteString, DllName, strlen(DllName), NULL);

		// Load our DLL by calling loadlibrary in the other process and passing our dll name
		CreateRemoteThread(Proc, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLib, (LPVOID)RemoteString, NULL, NULL);

		CloseHandle(Proc);
		return true;
	}

	static DWORD GetTargetThreadIDFromProcName(const char *ProcName)
	{
		// create a handle to the toolhelp32 library
		HANDLE thSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (thSnapShot == INVALID_HANDLE_VALUE)
		{
			//MessageBox(NULL, "Error: Unable to create toolhelp snapshot!", "2MLoader", MB_OK);
			printf("Error: Unable to create toolhelp snapshot!");
			return 0;
		}

		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(PROCESSENTRY32);

		// iterate over the currently running processes to find the one whose name matches `ProcName`
		BOOL retval = Process32First(thSnapShot, &pe);
		while (retval)
		{
			if (!strcmp(pe.szExeFile, ProcName))
			{
				// names match
				// close the handle and return the process id
				CloseHandle(thSnapShot);
				return pe.th32ProcessID;
			}
			retval = Process32Next(thSnapShot, &pe);
		}

		// unable to find the process
		// close the handle and return 0 signalling that we were unable to find the process id

		printf("Error: unable to find the process id!\n");

		CloseHandle(thSnapShot);

		return 0;
	}
};

int main(int argc, char **argv)
{
	if (argc > 2)
	{
		printf("injecting!\n");

		if (Injector::Inject(argv[1], argv[2]))
		{
			printf("injection successful!\n");
		}
		else
		{
			printf("injection failed\n");
			system("pause");
		}
		
		return 0;
	}

	printf("LoadLibrary Injector - F1ssi0N 2015\n"
		"Usage:\n"
		"    \"%s\" <process name> <dll name>\n"
		"Example:\n"
		"    \"%s\" hl2.exe basehook.dll\n\n"
		"For quick automation I recommend using a .bat file\n"
		"    call \"%s\" <process name> <dll name>\n"
		"Example:\n"
		"    call \"%s\" hl2.exe basehook.dll\n\n", argv[0], argv[0], argv[0], argv[0]);

	printf("Interactive mode:\nProcess name: ");

	char ProcName[256];

	scanf_s("%255s", ProcName, 255);

	printf("\nDll name: ");

	char DllName[MAX_PATH];

	scanf_s("%259s", DllName, MAX_PATH - 1);

	printf("injecting!\n");

	if (Injector::Inject(ProcName, DllName))
		printf("injection successful!\n");
	else
	{
		printf("injection failed\n");
		system("pause");
	}

}
