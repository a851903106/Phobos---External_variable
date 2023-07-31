#include <Phobos.h>

#include <Helpers/Macro.h>
#include <GameStrings.h>
#include <CCINIClass.h>
#include <Unsorted.h>
#include <Drawing.h>

#include "Utilities/Parser.h"
#include <Utilities/GeneralUtils.h>
#include <Utilities/Debug.h>
#include <Utilities/Patch.h>
#include <Utilities/Macro.h>

int Phobos::FlyStarAndSTSTL = 0;
bool Phobos::SelectBox = false;
HANDLE Phobos::hInstance = 0;

char Phobos::readBuffer[Phobos::readLength];
wchar_t Phobos::wideBuffer[Phobos::readLength];
const char Phobos::readDelims[4] = ",";

void Phobos::CmdLineParse(char** ppArgs, int nNumArgs)
{
	// > 1 because the exe path itself counts as an argument, too!
	for (int i = 1; i < nNumArgs; i++)
	{
		const char* pArg = ppArgs[i];

		if (_stricmp(pArg, "-Phobos") == 0)
		{
			Phobos::FlyStarAndSTSTL++;
		}

		if (_stricmp(pArg, "-FlyStar") == 0)
		{
			Phobos::FlyStarAndSTSTL++;
		}

		if (_stricmp(pArg, "-STSTL") == 0)
		{
			Phobos::FlyStarAndSTSTL++;
		}
	}
}

void Phobos::ExeRun()
{
	Patch::ApplyStatic();
}

void Phobos::ExeTerminate()
{
	Console::Release();
}

// =============================
// hooks

bool __stdcall DllMain(HANDLE hInstance, DWORD dwReason, LPVOID v)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		Phobos::hInstance = hInstance;
	}
	return true;
}

DEFINE_HOOK(0x7CD810, ExeRun, 0x9)
{
	Phobos::ExeRun();

	return 0;
}

void NAKED _ExeTerminate()
{
	// Call WinMain
	SET_REG32(EAX, 0x6BB9A0);
	CALL(EAX);
	PUSH_REG(EAX);

	Phobos::ExeTerminate();

	// Jump back
	POP_REG(EAX);
	SET_REG32(EBX, 0x7CD8EF);
	__asm {jmp ebx};
}
DEFINE_JUMP(LJMP, 0x7CD8EA, GET_OFFSET(_ExeTerminate));

DEFINE_HOOK(0x52F639, _YR_CmdLineParse, 0x5)
{
	GET(char**, ppArgs, ESI);
	GET(int, nNumArgs, EDI);

	Phobos::CmdLineParse(ppArgs, nNumArgs);

	return 0;
}
