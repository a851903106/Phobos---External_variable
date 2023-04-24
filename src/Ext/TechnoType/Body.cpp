#include "Body.h"

#include <TechnoTypeClass.h>
#include <Ext/Techno/Body.h>

template<> const DWORD Extension<TechnoTypeClass>::Canary = 0x11111111;
TechnoTypeExt::ExtContainer TechnoTypeExt::ExtMap;

void TechnoTypeExt::ExtData::Initialize() { }

// =============================
// load / save

void TechnoTypeExt::ExtData::LoadFromINIFile(CCINIClass* const pINI)
{
	auto pThis = this->OwnerObject();
	const char* pSection = pThis->ID;

	if (!pINI->GetSection(pSection))
		return;

	INI_EX exINI(pINI);

	this->ExVartoVar.Read(exINI, pSection, "ExVartoVar");
	this->ExVartoVar_File.Read(pINI, pSection, "ExVartoVar.File");
	this->ExVartoVar_Section.Read(pINI, pSection, "ExVartoVar.Section");
	this->ExVartoVar_Key.Read(pINI, pSection, "ExVartoVar.Key");

	for (size_t i = 0; ; ++i)
	{
		char type[64];
		_snprintf_s(type, sizeof(type), "ExVartoVar.SW%d", i);

		PhobosFixedString<32U> SWtype;
		SWtype.Read(pINI, pSection, type);

		if (strcmp(SWtype.data(), "") == 0)
			break;

		int SWIdx = SuperWeaponTypeClass::FindIndex(SWtype);
		this->ExVartoVar_SW.push_back(SuperWeaponTypeClass::Array()->GetItem(SWIdx));
	}

	this->VartoExVar.Read(exINI, pSection, "VartoExVar");
	this->VartoExVar_File.Read(pINI, pSection, "VartoExVar.File");
	this->VartoExVar_Section.Read(pINI, pSection, "VartoExVar.Section");
	this->VartoExVar_Key.Read(pINI, pSection, "VartoExVar.Key");
	this->VartoExVar_Value.Read(pINI, pSection, "VartoExVar.Value");
	this->VartoExVar_IsInt.Read(exINI, pSection, "VartoExVar.IsInt");
	this->VartoExVar_Number.Read(exINI, pSection, "VartoExVar.Number");
}

template <typename T>
void TechnoTypeExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->ExVartoVar)
		.Process(this->ExVartoVar_File)
		.Process(this->ExVartoVar_Section)
		.Process(this->ExVartoVar_Key)
		.Process(this->ExVartoVar_SW)

		.Process(this->VartoExVar)
		.Process(this->VartoExVar_File)
		.Process(this->VartoExVar_Section)
		.Process(this->VartoExVar_Key)
		.Process(this->VartoExVar_Value)
		.Process(this->VartoExVar_IsInt)
		.Process(this->VartoExVar_Number)
		;
}
void TechnoTypeExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TechnoTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoTypeExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TechnoTypeClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container

TechnoTypeExt::ExtContainer::ExtContainer() : Container("TechnoTypeClass") { }
TechnoTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x711835, TechnoTypeClass_CTOR, 0x5)
{
	GET(TechnoTypeClass*, pItem, ESI);

	TechnoTypeExt::ExtMap.FindOrAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x711AE0, TechnoTypeClass_DTOR, 0x5)
{
	GET(TechnoTypeClass*, pItem, ECX);

	TechnoTypeExt::ExtMap.Remove(pItem);

	return 0;
}

DEFINE_HOOK_AGAIN(0x716DC0, TechnoTypeClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x7162F0, TechnoTypeClass_SaveLoad_Prefix, 0x6)
{
	GET_STACK(TechnoTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TechnoTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x716DAC, TechnoTypeClass_Load_Suffix, 0xA)
{
	TechnoTypeExt::ExtMap.LoadStatic();

	return 0;
}

DEFINE_HOOK(0x717094, TechnoTypeClass_Save_Suffix, 0x5)
{
	TechnoTypeExt::ExtMap.SaveStatic();

	return 0;
}

DEFINE_HOOK_AGAIN(0x716132, TechnoTypeClass_LoadFromINI, 0x5)
DEFINE_HOOK(0x716123, TechnoTypeClass_LoadFromINI, 0x5)
{
	GET(TechnoTypeClass*, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x380);

	TechnoTypeExt::ExtMap.LoadFromINI(pItem, pINI);

	return 0;
}
