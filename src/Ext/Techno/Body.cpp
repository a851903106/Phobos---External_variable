#include "Body.h"

#include <InfantryClass.h>
#include <UnitClass.h>
#include <AircraftClass.h>
#include <BuildingClass.h>
#include <HouseClass.h>
#include <WeaponTypeClass.h>
#include <WarheadTypeClass.h>
#include <AirstrikeClass.h>
#include <SpawnManagerClass.h>
#include <LocomotionClass.h>

template<> const DWORD Extension<TechnoClass>::Canary = 0x55555555;
TechnoExt::ExtContainer TechnoExt::ExtMap;

void TechnoExt::ExtData::LoadConvert()
{
	const auto pThis = this->OwnerObject();
	const auto pType = pThis->GetTechnoType();
	const auto pTypeExt = this->TypeExtData;
	const auto pPassenger = pThis->Passengers.GetFirstPassenger();
	const auto pPassengerType = pPassenger->GetTechnoType();

	if (!pPassenger || !pPassengerType || pType->WhatAmI() != AbstractType::UnitType)
		return;

	if (!pTypeExt->Convert_Loads.empty())
	{
		for (size_t i = 0; i < pTypeExt->Convert_Loads.size(); i++)
		{
			const auto pLoads = pTypeExt->Convert_Loads[i];

			if (pLoads.Contains(pPassengerType) && pTypeExt->Convert_Types.size() > i)
			{
				const auto type = (pTypeExt->Convert_Types[i]);

				if (!type || type == pType)
					continue;

				if (TechnoExt::ConvertToType(abstract_cast<FootClass*>(pThis), type))
				{
					if (type->Gunner)
						abstract_cast<FootClass*>(pThis)->ReceiveGunner(pPassenger);

					return;
				}
			}
		}
	}

	if (const auto type = pTypeExt->Convert_Load.Get())
	{
		if (!type || type == pType)
			return;

		if (TechnoExt::ConvertToType(abstract_cast<FootClass*>(pThis), type))
		{
			if (type->Gunner)
				abstract_cast<FootClass*>(pThis)->ReceiveGunner(pPassenger);
		}
	}
}

void TechnoExt::ExtData::UnloadConvert()
{
	const auto pThis = this->OwnerObject();
	const auto pType = pThis->GetTechnoType();
	const auto pTypeExt = this->TypeExtData;

	if (pType->WhatAmI() != AbstractType::UnitType)
		return;

	if (const auto type = pTypeExt->Convert_Unload.Get())
	{
		if (!type || type == pType)
			return;

		TechnoExt::ConvertToType(abstract_cast<FootClass*>(pThis), type);
	}
}

// Original link : github.com/Phobos-developers/Phobos/pull/841/files#diff-f66a73e71373ecbfe389cfba5d43c80cb5cb3b3050ec6a9875c392a170591ffb
// Feature for common usage : TechnoType conversion -- Trsdy
// BTW, who said it was merely a Type pointer replacement and he could make a better one than Ares?
bool TechnoExt::ConvertToType(FootClass* pThis, TechnoTypeClass* pToType)
{
	// In case not using Ares 3.0. Only update necessary vanilla properties
	AbstractType rtti;
	TechnoTypeClass** nowTypePtr;

	// Different types prohibited
	switch (pThis->WhatAmI())
	{
	case AbstractType::Infantry:
		nowTypePtr = reinterpret_cast<TechnoTypeClass**>(&(static_cast<InfantryClass*>(pThis)->Type));
		rtti = AbstractType::InfantryType;
		break;
	case AbstractType::Unit:
		nowTypePtr = reinterpret_cast<TechnoTypeClass**>(&(static_cast<UnitClass*>(pThis)->Type));
		rtti = AbstractType::UnitType;
		break;
	case AbstractType::Aircraft:
		nowTypePtr = reinterpret_cast<TechnoTypeClass**>(&(static_cast<AircraftClass*>(pThis)->Type));
		rtti = AbstractType::AircraftType;
		break;
	default:
		Debug::Log("%s is not FootClass, conversion not allowed\n", pToType->get_ID());
		return false;
	}

	if (pToType->WhatAmI() != rtti)
	{
		Debug::Log("Incompatible types between %s and %s\n", pThis->get_ID(), pToType->get_ID());
		return false;
	}

	// Detach CLEG targeting
	auto tempUsing = pThis->TemporalImUsing;
	if (tempUsing && tempUsing->Target)
		tempUsing->Detach();

	HouseClass* const pOwner = pThis->Owner;

	// Remove tracking of old techno
	if (!pThis->InLimbo)
		pOwner->RegisterLoss(pThis, false);
	pOwner->RemoveTracking(pThis);

	int oldHealth = pThis->Health;

	// Generic type-conversion
	TechnoTypeClass* prevType = *nowTypePtr;
	*nowTypePtr = pToType;

	// Readjust health according to percentage
	pThis->SetHealthPercentage((double)(oldHealth) / (double)prevType->Strength);
	pThis->EstimatedHealth = pThis->Health;

	// Add tracking of new techno
	pOwner->AddTracking(pThis);
	if (!pThis->InLimbo)
		pOwner->RegisterGain(pThis, false);
	pOwner->RecheckTechTree = true;

	// Update Ares AttachEffects -- skipped
	// Ares RecalculateStats -- skipped

	// Adjust ammo
	pThis->Ammo = Math::min(pThis->Ammo, pToType->Ammo);
	// Ares ResetSpotlights -- skipped

	// Adjust ROT
	if (rtti == AbstractType::AircraftType)
		pThis->SecondaryFacing.SetROT(pToType->ROT);
	else
		pThis->PrimaryFacing.SetROT(pToType->ROT);
	// Adjust Ares TurretROT -- skipped
	//  pThis->SecondaryFacing.SetROT(TechnoTypeExt::ExtMap.Find(pToType)->TurretROT.Get(pToType->ROT));

	// Locomotor change, referenced from Ares 0.A's abduction code, not sure if correct, untested
	CLSID nowLocoID;
	ILocomotion* iloco = pThis->Locomotor.get();
	const auto& toLoco = pToType->Locomotor;
	if ((SUCCEEDED(static_cast<LocomotionClass*>(iloco)->GetClassID(&nowLocoID)) && nowLocoID != toLoco))
	{
		// because we are throwing away the locomotor in a split second, piggybacking
		// has to be stopped. otherwise the object might remain in a weird state.
		while (LocomotionClass::End_Piggyback(pThis->Locomotor));
		// throw away the current locomotor and instantiate
		// a new one of the default type for this unit.
		if (auto newLoco = LocomotionClass::CreateInstance(toLoco))
		{
			newLoco->Link_To_Object(pThis);
			pThis->Locomotor = std::move(newLoco);
		}
	}

	// TODO : Jumpjet locomotor special treatement, some brainfart, must be uncorrect, HELP ME!
	const auto& jjLoco = LocomotionClass::CLSIDs::Jumpjet();
	if (pToType->BalloonHover && pToType->DeployToLand && prevType->Locomotor != jjLoco && toLoco == jjLoco)
		pThis->Locomotor->Move_To(pThis->Location);

	TechnoExt::CheckWeapons(pThis, pToType);

	return true;
}

void TechnoExt::CheckWeapons(FootClass* pThis, TechnoTypeClass* pType)
{
	if (const auto pWeaponType = pType->GetWeapon(0, false).WeaponType)
		TechnoExt::CheckWeapon(pThis, pWeaponType);

	if (const auto pWeaponType = pType->GetWeapon(0, true).WeaponType)
		TechnoExt::CheckWeapon(pThis, pWeaponType);

	if (const auto pWeaponType = pType->GetWeapon(1, false).WeaponType)
		TechnoExt::CheckWeapon(pThis, pWeaponType);

	if (const auto pWeaponType = pType->GetWeapon(1, true).WeaponType)
		TechnoExt::CheckWeapon(pThis, pWeaponType);

	for (int i = 0; i < pType->WeaponCount; i++)
	{
		if (const auto pWeaponType = pType->GetWeapon(i, false).WeaponType)
			TechnoExt::CheckWeapon(pThis, pWeaponType);

		if (const auto pWeaponType = pType->GetWeapon(i, true).WeaponType)
			TechnoExt::CheckWeapon(pThis, pWeaponType);
	}
}

void TechnoExt::CheckWeapon(FootClass* pThis, WeaponTypeClass* pWeapon)
{
	if (!pThis || !pWeapon || !pWeapon->Warhead)
		return;

	const auto pTechno = abstract_cast<TechnoClass*>(pThis);

	if (!pTechno)
		return;

	if (pWeapon->Warhead->MindControl && !pThis->CaptureManager)
	{
		pThis->CaptureManager = GameCreate<CaptureManagerClass>(pTechno, pWeapon->Damage, pWeapon->InfiniteMindControl);
	}

	if (pWeapon->Warhead->Temporal && !pThis->TemporalImUsing)
	{
		pThis->TemporalImUsing = GameCreate<TemporalClass>(pTechno);
	}

	if (pWeapon->Warhead->Parasite && !pThis->ParasiteImUsing)
	{
		pThis->ParasiteImUsing = GameCreate<ParasiteClass>(pThis);
	}

	if (pWeapon->Warhead->Airstrike && !pThis->Airstrike)
	{
		pThis->Airstrike = GameCreate<AirstrikeClass>(pTechno);
	}

	if (pWeapon->Spawner && !pThis->SpawnManager)
	{
		pThis->SpawnManager = GameCreate<SpawnManagerClass>(pTechno, pTechno->GetTechnoType()->Spawns, pTechno->GetTechnoType()->SpawnsNumber,
			pTechno->GetTechnoType()->SpawnRegenRate, pTechno->GetTechnoType()->SpawnReloadRate);
	}
}

// =============================
// load / save

template <typename T>
void TechnoExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->TypeExtData)
		.Process(this->PassengerNum)
		;
}

void TechnoExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TechnoClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TechnoClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool TechnoExt::LoadGlobals(PhobosStreamReader& Stm)
{
	return Stm
		.Success();
}

bool TechnoExt::SaveGlobals(PhobosStreamWriter& Stm)
{
	return Stm
		.Success();
}

// =============================
// container

TechnoExt::ExtContainer::ExtContainer() : Container("TechnoClass") { }

TechnoExt::ExtContainer::~ExtContainer() = default;

void TechnoExt::ExtContainer::InvalidatePointer(void* ptr, bool bRemoved) { }

// =============================
// container hooks

DEFINE_HOOK(0x6F3260, TechnoClass_CTOR, 0x5)
{
	GET(TechnoClass*, pItem, ESI);

	TechnoExt::ExtMap.FindOrAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x6F4500, TechnoClass_DTOR, 0x5)
{
	GET(TechnoClass*, pItem, ECX);

	TechnoExt::ExtMap.Remove(pItem);

	return 0;
}

DEFINE_HOOK_AGAIN(0x70C250, TechnoClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x70BF50, TechnoClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(TechnoClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TechnoExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x70C249, TechnoClass_Load_Suffix, 0x5)
{
	TechnoExt::ExtMap.LoadStatic();

	return 0;
}

DEFINE_HOOK(0x70C264, TechnoClass_Save_Suffix, 0x5)
{
	TechnoExt::ExtMap.SaveStatic();

	return 0;
}
