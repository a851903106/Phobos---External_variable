#pragma once
#include <Phobos.version.h>
#include <Windows.h>

class CCINIClass;
class AbstractClass;

class Phobos
{
public:
	static int FlyStarAndSTSTL;
	static bool SelectBox;
	static void Phobos::CmdLineParse(char** ppArgs, int nNumArgs);
	static void ExeRun();
	static void ExeTerminate();

	//variables
	static HANDLE hInstance;

	static const size_t readLength = 2048;
	static char readBuffer[readLength];
	static wchar_t wideBuffer[readLength];
	static const char readDelims[4];

#ifdef DEBUG
	static bool DetachFromDebugger();
#endif
	static void Clear();
	static void PointerGotInvalid(AbstractClass* const pInvalid, bool const removed);
	static HRESULT SaveGameData(IStream* pStm);
	static void LoadGameData(IStream* pStm);
};
