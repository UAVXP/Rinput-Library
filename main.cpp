/*
	RInput allows you to override low definition windows mouse input with high definition mouse input.

	RInput Copyright (C) 2012, J. Dijkstra (abort@digitalise.net)

	This file is part of RInput.

    RInput is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RInput is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with RInput.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"

int __stdcall DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	switch(dwReason)
	{
		case DLL_PROCESS_ATTACH:
			// No need for a threaded entry
			if (!DisableThreadLibraryCalls(hInstance)) return 0;
			g_hInstance = hInstance;

#ifdef _DEBUG
			OutputDebugString("Loaded RInput");
#endif

			break;
		case DLL_PROCESS_DETACH:
				CRawInput::hookLibrary(false);
				CRawInput::unload();
#ifdef _DEBUG
			OutputDebugString("Unloaded RInput");
#endif
			break;
	}

	return 1;
}

extern "C" __declspec(dllexport) void entryPoint()
{
#ifdef _DEBUG
	OutputDebugString("entryPoint called");
#endif

	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, EVENTNAME);
	if (!hEvent)
		displayError(L"Could not open interprocess communication event.");

	WCHAR pwszError[ERROR_BUFFER_SIZE];

	if (!validateVersion())
		displayError(L"You must at least have Microsoft Windows XP to use this library.");

	if (!CRawInput::initialize(pwszError))
		displayError(pwszError);

	if (!CRawInput::hookLibrary(true))
		displayError(L"Failed to hook Windows API cursor functions.");

	// Signal the injector that the injection and hooking are successful
	if (!SetEvent(hEvent))
		displayError(L"Failed to signal the initialization event.");

	CloseHandle(hEvent);

	if (!CRawInput::pollInput())
		displayError(L"Failed to poll mouse input");

#ifdef _DEBUG
	OutputDebugString("Finished entryPoint");
#endif
}

// Validate that we are working with Windows XP or higher (required for raw input)
inline bool validateVersion()
{
	DWORD dwVersion = GetVersion();
	double fCompareVersion = LOBYTE(LOWORD(dwVersion)) + 0.1 * HIBYTE(LOWORD(dwVersion));
	return (dwVersion && fCompareVersion >= 5.1);
}

void displayError(WCHAR* pwszError)
{
	MessageBoxW(NULL, pwszError, L"Raw Input error!", MB_ICONERROR | MB_OK);
	CRawInput::hookLibrary(false);
	unloadLibrary();
}

void unloadLibrary()
{
	__asm {
		push -2
		push 0
		push g_hInstance
		mov eax, TerminateThread
		push eax
		mov eax, FreeLibrary
		jmp eax
	}
}