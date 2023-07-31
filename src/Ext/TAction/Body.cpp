#include "Body.h"

#include <SessionClass.h>
#include <MessageListClass.h>
#include <HouseClass.h>
#include <CRT.h>
#include <SpawnManagerClass.h>
#include <ScenarioClass.h>
#include <SuperClass.h>
#include <SuperWeaponTypeClass.h>
#include <TagTypeClass.h>
#include <Helpers/Macro.h>
#include <WWMessageBox.h>

#include <Ext/Anim/Body.h>

#include <Utilities/Helpers.Alex.h>
#include <Utilities/SavegameDef.h>

//Static init
template<> const DWORD Extension<TActionClass>::Canary = 0x91919191;
TActionExt::ExtContainer TActionExt::ExtMap;

// =============================
// load / save

template <typename T>
void TActionExt::ExtData::Serialize(T& Stm)
{
	Stm;
}

void TActionExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TActionClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TActionExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TActionClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool TActionExt::Execute(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location, bool& bHandled)
{
	if (!SessionClass::Instance->IsCampaign())
		return false;

	bHandled = true;

	// Phobos
	switch (static_cast<PhobosTriggerAction>(pThis->ActionKind))
	{
	case PhobosTriggerAction::TechnoUninit:
		return TActionExt::TechnoUninit(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::MessageForSpecifiedColor:
		return TActionExt::MessageForSpecifiedColor(pThis, pHouse, pObject, pTrigger, location);
	
	case PhobosTriggerAction::FireSuperWeapon:
		return TActionExt::FireSuperWeapon(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::CreateAnimation:
		return TActionExt::CreateAnimation(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::DeleteAnimation:
		return TActionExt::DeleteAnimation(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SelectBox:
		return TActionExt::SelectBox(pThis, pHouse, pObject, pTrigger, location);
	default:
		bHandled = false;
		return true;
	}
}

bool TActionExt::TechnoUninit(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	DynamicVectorClass<TechnoClass*> pTechnoList;

	for (int i = 0; i < TechnoClass::Array()->Count; i++)
	{
		const auto pTechno = TechnoClass::Array()->GetItem(i);
		if (!pTechno)
			continue;

		const auto pTag = pTechno->AttachedTag;
		if (!pTag || !pTag->ContainsTrigger(pTrigger))
			continue;

		pTechnoList.AddItem(pTechno);
	}

	for (const auto pTechno : pTechnoList)
	{
		TActionExt::ClearManager(pTechno);
		pTechno->KillPassengers(pTechno);
		pTechno->vt_entry_3A0(); // Stun? what is this?
		pTechno->Limbo();
		pTechno->UnInit();
	}

	pTechnoList.Clear();
	return true;
}

bool TActionExt::MessageForSpecifiedColor(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int ColorIndex = pHouse->Type->ColorSchemeIndex;
	char Text[32];
	char* Color = NULL;
	strcpy_s(Text, pThis->Text);
	strtok_s(Text, "@", &Color);

	if (strcmp(Color, "") != 0)
		ColorIndex = ColorScheme::FindIndex(Color);

	MessageListClass::Instance->PrintMessage(StringTable::LoadString(Text), RulesClass::Instance->MessageDelay, ColorIndex);

	return true;
}

bool TActionExt::FireSuperWeapon(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	SuperWeaponTypeClass* pSWType = SuperWeaponTypeClass::Find(pThis->Text);

	if (pSWType == nullptr)
		pSWType = SuperWeaponTypeClass::Array()->GetItem(atoi(pThis->Text));

	if (pSWType == nullptr)
		return true;

	HouseClass* vHouse = pHouse;
	if (pThis->Param3 >= 0)
	{
		if (HouseClass::Index_IsMP(pThis->Param3))
			vHouse = HouseClass::FindByIndex(pThis->Param3);
		else
			vHouse = HouseClass::FindByCountryIndex(pThis->Param3);
	}

	if (vHouse == nullptr)
		return true;

	CellStruct targetLocation = { (short)pThis->Param4, (short)pThis->Param5 };
	if (pThis->Param4 < 0)
		targetLocation.X = (short)ScenarioClass::Instance->Random.RandomRanged(0, MapClass::Instance->MapCoordBounds.Right);

	if (pThis->Param5 < 0)
		targetLocation.Y = (short)ScenarioClass::Instance->Random.RandomRanged(0, MapClass::Instance->MapCoordBounds.Bottom);

	SuperClass* pSuper = GameCreate<SuperClass>(pSWType, vHouse);
	int oldstart = pSuper->RechargeTimer.StartTime;
	int oldleft = pSuper->RechargeTimer.TimeLeft;
	pSuper->SetReadiness(true);
	pSuper->Launch(targetLocation, false);
	pSuper->Reset();
	pSuper->RechargeTimer.StartTime = oldstart;
	pSuper->RechargeTimer.TimeLeft = oldleft;

	if (pThis->Param6 > 0)
	{
		if (const auto pSuper2 = vHouse->Supers.GetItem(pSWType->GetArrayIndex()))
			pSuper2->Reset();
	}

	return true;
}

bool TActionExt::CreateAnimation(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	char animText[32];
	char* height = NULL;

	strcpy_s(animText, pThis->Text);
	strtok_s(animText, "@", &height);

	auto pAnimType = AnimTypeClass::Find(animText);

	if (pAnimType == nullptr)
		pAnimType = AnimTypeClass::Array()->GetItem(atoi(animText));

	if (pAnimType == nullptr)
		return true;

	if (pThis->Param3 < 0)
		return true;

	if (pThis->Param5 < 0 || pThis->Param6 < 0)
		return true;

	HouseClass* vHouse = pHouse;
	if (pThis->Param4 >= 0)
	{
		if (HouseClass::Index_IsMP(pThis->Param4))
			vHouse = HouseClass::FindByIndex(pThis->Param4);
		else
			vHouse = HouseClass::FindByCountryIndex(pThis->Param4);
	}

	if (vHouse == nullptr)
		return true;

	CellStruct targetLocation = { (short)pThis->Param5, (short)pThis->Param6 };
	const auto pCell = MapClass::Instance->TryGetCellAt(targetLocation);
	if (!pCell)
		return true;

	if (const auto pAnim = GameCreate<AnimClass>(pAnimType, pCell->GetCoords()))
	{
		pAnim->Owner = vHouse;
		pAnim->Location.Z += atoi(height);

		if (const auto pAnimExt = AnimExt::ExtMap.Find(pAnim))
		{
			pAnimExt->ID = pThis->Param3;
		}
	}

	return true;
}

bool TActionExt::DeleteAnimation(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	if (pThis->Param3 < 0)
		return true;

	DynamicVectorClass<AnimClass*> AnimList;

	for (int i = 0; i < AnimClass::Array()->Count; i++)
	{
		const auto pAnim = AnimClass::Array()->GetItem(i);
		if (!pAnim)
			continue;

		const auto pAnimExt = AnimExt::ExtMap.Find(pAnim);
		if (!pAnimExt)
			continue;

		if (pAnimExt->ID != pThis->Param3)
			continue;

		AnimList.AddItem(pAnim);
	}

	for (const auto pAnim : AnimList)
	{
		pAnim->TimeToDie = true;
		pAnim->Limbo();
		pAnim->UnInit();
	}

	return true;
}

bool TActionExt::SelectBox(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	char Title[32];
	char* Button1 = NULL;
	char* Button2 = NULL;
	char* Button3 = NULL;

	strcpy_s(Title, pThis->Text);
	strtok_s(Title, "@", &Button1);
	strtok_s(Button1, "@", &Button2);
	strtok_s(Button2, "@", &Button3);

	Phobos::SelectBox = true;
	int buttonCount = 0;
	WWMessageBox::Result result = WWMessageBox::Result::Button1;

	if (strcmp(Button1, "0") == 0 || strcmp(Button1, "") == 0)
	{
		buttonCount = 1;
		result = WWMessageBox::Instance->Process(
			StringTable::LoadStringA(Title),
			StringTable::LoadStringA("TXT_OK"),
			0,
			0);
	}
	else
	{
		if (strcmp(Button2, "0") == 0 || strcmp(Button2, "") == 0)
		{
			buttonCount = 1;
			result = WWMessageBox::Instance->Process(
				StringTable::LoadStringA(Title),
				StringTable::LoadStringA(Button1),
				0,
				0);
		}
		else
		{
			if (strcmp(Button3, "0") == 0 || strcmp(Button3, "") == 0)
			{
				buttonCount = 2;
				result = WWMessageBox::Instance->Process(
				StringTable::LoadStringA(Title),
					StringTable::LoadStringA(Button1),
					StringTable::LoadStringA(Button2),
					0);
			}
			else
			{
				buttonCount = 3;
				result = WWMessageBox::Instance->Process(
				StringTable::LoadStringA(Title),
				StringTable::LoadStringA(Button1),
				StringTable::LoadStringA(Button3),
				StringTable::LoadStringA(Button2));
			}
		}
	}

	Phobos::SelectBox = false;
	if (buttonCount <= 1)
		return true;

	int value = (int)result;
	int SWIndex = -1;

	if (buttonCount == 2)
	{
		switch (value)
		{
		case 0:
			SWIndex = pThis->Param3;
			break;
		default:
			break;
		}
	}
	else if (buttonCount == 3)
	{
		switch (value)
		{
		case 0:
			SWIndex = pThis->Param3;
			break;
		case 2:
			SWIndex = pThis->Param4;
			break;
		default:
			break;
		}
	}

	if (SWIndex < 0)
		return true;

	const auto pSWType = SuperWeaponTypeClass::Array()->GetItem(SWIndex);
	if (pSWType == nullptr)
		return true;

	CellStruct targetLocation = { (short)pThis->Param5, (short)pThis->Param6 };
	if (pThis->Param5 < 0)
		targetLocation.X = (short)ScenarioClass::Instance->Random.RandomRanged(0, MapClass::Instance->MapCoordBounds.Right);

	if (pThis->Param6 < 0)
		targetLocation.Y = (short)ScenarioClass::Instance->Random.RandomRanged(0, MapClass::Instance->MapCoordBounds.Bottom);

	SuperClass* pSuper = GameCreate<SuperClass>(pSWType, pHouse);
	int oldstart = pSuper->RechargeTimer.StartTime;
	int oldleft = pSuper->RechargeTimer.TimeLeft;
	pSuper->SetReadiness(true);
	pSuper->Launch(targetLocation, false);
	pSuper->Reset();
	pSuper->RechargeTimer.StartTime = oldstart;
	pSuper->RechargeTimer.TimeLeft = oldleft;

	return true;
}

void TActionExt::ClearManager(TechnoClass* pThis)
{
	if (pThis->CaptureManager)
	{
		pThis->CaptureManager->FreeAll();
	}

	if (pThis->SpawnManager)
	{
		pThis->SpawnManager->KillNodes();
	}

	if (pThis->SlaveManager)
	{
		pThis->SlaveManager->Killed(nullptr, pThis->Owner);
	}

	if (pThis->TemporalImUsing && pThis->TemporalImUsing->Owner && pThis->TemporalImUsing->Target)
	{
		pThis->TemporalImUsing->Detach();
	}

	if (pThis->TemporalTargetingMe && pThis->TemporalTargetingMe->Owner && pThis->TemporalTargetingMe->Target)
	{
		pThis->TemporalTargetingMe->Detach();
	}

	if (const auto pFoot = abstract_cast<FootClass*>(pThis))
	{
		if (pFoot->ParasiteImUsing && pFoot->ParasiteImUsing->Victim)
		{
			pFoot->ParasiteImUsing->ExitUnit();
		}

		if (pFoot->ParasiteEatingMe)
		{
			pFoot->ParasiteEatingMe->ParasiteImUsing->ExitUnit();
			pFoot->ParasiteEatingMe = nullptr;
		}
	}
}

// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") { }

TActionExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

/*
DEFINE_HOOK(0x6DD176, TActionClass_CTOR, 0x5)
{
	GET(TActionClass*, pItem, ESI);

	TActionExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x6E4761, TActionClass_SDDTOR, 0x6)
{
	GET(TActionClass*, pItem, ESI);

	TActionExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x6E3E30, TActionClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x6E3DB0, TActionClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(TActionClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TActionExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x6E3E29, TActionClass_Load_Suffix, 0x4)
{
	TActionExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x6E3E4A, TActionClass_Save_Suffix, 0x3)
{
	TActionExt::ExtMap.SaveStatic();
	return 0;
}
*/
