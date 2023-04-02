#include "Body.h"

template<> const DWORD Extension<TechnoClass>::Canary = 0x55555555;
TechnoExt::ExtContainer TechnoExt::ExtMap;

void TechnoExt::ExtData::DeleteSelf()
{
	const auto pThis = this->OwnerObject();
	const auto pHouse = HouseClass::FindSpecial();

	pThis->SetOwningHouse(pHouse);
	pThis->Limbo();
	pThis->UnInit();
}

void TechnoExt::ExtData::FireSuperWeapon(SuperWeaponTypeClass* pSWType, HouseClass* pHouse, CoordStruct coords)
{
	if (const auto pSuper = pHouse->Supers.GetItem(SuperWeaponTypeClass::Array->FindItemIndex(pSWType)))
	{
		const auto cell = CellClass::Coord2Cell(coords);
		int oldstart = pSuper->RechargeTimer.StartTime;
		int oldleft = pSuper->RechargeTimer.TimeLeft;
		const auto MouseIdx = Unsorted::CurrentSWType.get();

		pSuper->SetReadiness(true);
		pSuper->Launch(cell, true);
		pSuper->Reset();

		Unsorted::CurrentSWType = MouseIdx;
		pSuper->RechargeTimer.StartTime = oldstart;
		pSuper->RechargeTimer.TimeLeft = oldleft;
	}
}

// =============================
// load / save

template <typename T>
void TechnoExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->TypeExtData)
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
