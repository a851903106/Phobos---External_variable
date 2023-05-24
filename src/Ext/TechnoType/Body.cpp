#include "Body.h"

#include <TechnoTypeClass.h>
#include <Ext/Techno/Body.h>

template<> const DWORD Extension<TechnoTypeClass>::Canary = 0x11111111;
TechnoTypeExt::ExtContainer TechnoTypeExt::ExtMap;

void TechnoTypeExt::ExtData::Initialize() { }

void TechnoTypeExt::GetWeapons(TechnoTypeClass* pThis, INI_EX& exINI, const char* pSection,
	std::vector<WeaponTypeClass*>& n, std::vector<WeaponTypeClass*>& nE)
{
	char tempBuffer[32];
	auto weaponCount = pThis->WeaponCount;
	if (weaponCount <= 0)
		return;

	for (int i = 0; i < weaponCount; i++)
	{
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "Weapon%d", i + 1);
		Nullable<WeaponTypeClass*> Weapon;
		Weapon.Read(exINI, pSection, tempBuffer, true);

		_snprintf_s(tempBuffer, sizeof(tempBuffer), "EliteWeapon%d", i + 1);
		Nullable<WeaponTypeClass*> EliteWeapon;
		EliteWeapon.Read(exINI, pSection, tempBuffer, true);

		if (!EliteWeapon.isset())
			EliteWeapon = Weapon;

		n.push_back(Weapon.Get());
		nE.push_back(EliteWeapon.Get());
	}
}

// =============================
// load / save

void TechnoTypeExt::ExtData::LoadFromINIFile(CCINIClass* const pINI)
{
	auto pThis = this->OwnerObject();
	const char* pSection = pThis->ID;

	if (!pINI->GetSection(pSection))
		return;

	INI_EX exINI(pINI);

	TechnoTypeExt::GetWeapons(pThis, exINI, pSection, this->Weapons, this->EliteWeapons);

	this->UseConvert.Read(exINI, pSection, "UseConvert");

	if (pThis->WhatAmI() == AbstractType::UnitType && this->UseConvert.Get())
	{
		this->Convert_Load.Read(exINI, pSection, "Convert.Load", true);
		this->Convert_Unload.Read(exINI, pSection, "Convert.Unload", true);

		for (int i = 0;; i++)
		{
			char text[64];
			NullableVector<TechnoTypeClass*> Types;

			sprintf_s(text, sizeof(text), "Convert.Load%d", i + 1);
			Types.Read(exINI, pSection, text);

			if (Types.empty())
				break;
			else
				this->Convert_Loads.push_back(Types);

			Nullable<UnitTypeClass*> Type;
			sprintf_s(text, sizeof(text), "Convert.Type%d", i + 1);
			Type.Read(exINI, pSection, text, true);

			if (!Type.Get())
				break;
			else
				this->Convert_Types.push_back(Type);
		}
	}
}

template <typename T>
void TechnoTypeExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->UseConvert)
		.Process(this->Convert_Load)
		.Process(this->Convert_Unload)
		.Process(this->Convert_Loads)
		.Process(this->Convert_Types)
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
