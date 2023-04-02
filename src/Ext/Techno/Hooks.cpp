#include "Body.h"

#include <Utilities/Macro.h>

DEFINE_HOOK(0x6F9E50, TechnoClass_AI, 0x5)
{
	GET(TechnoClass*, pThis, ECX);

	// Do not search this up again in any functions called here because it is costly for performance - Starkku
	auto pExt = TechnoExt::ExtMap.Find(pThis);
	auto pType = pThis->GetTechnoType();

	// Set only if unset or type is changed
	// Notice that Ares may handle type conversion in the same hook here, which is executed right before this one thankfully
	if (!pExt->TypeExtData || pExt->TypeExtData->OwnerObject() != pType)
		pExt->TypeExtData = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	if (SessionClass::Instance->IsCampaign())
	{
		if (const auto pTypeExt = pExt->TypeExtData)
		{
			bool Delete = false;
			if (pTypeExt->ExVartoVar)
			{
				CCINIClass* pINI = Phobos::OpenConfig(pTypeExt->ExVartoVar_File);
				const auto number = pINI->ReadInteger(pTypeExt->ExVartoVar_Section, pTypeExt->ExVartoVar_Key, 0);

				if (pTypeExt->ExVartoVar_SW.size() > number)
				{
					if (const auto pSWType = pTypeExt->ExVartoVar_SW[number])
						pExt->FireSuperWeapon(pSWType, pThis->Owner, pThis->GetCoords());
				}

				Phobos::CloseConfig(pINI);

				Delete = true;
			}

			if (pTypeExt->VartoExVar)
			{
				CCINIClass* pINI = GameCreate<CCINIClass>();
				CCFileClass* cfg = GameCreate<CCFileClass>(pTypeExt->VartoExVar_File);

				if (pINI)
				{
					if (cfg)
					{
						pINI->ReadCCFile(cfg);

						pINI->WriteString(pTypeExt->VartoExVar_Section, pTypeExt->VartoExVar_Key, pTypeExt->VartoExVar_Value);
						pINI->WriteCCFile(cfg);
					}

					GameDelete(cfg);
					GameDelete(pINI);
				}

				pINI = nullptr;

				Delete = true;
			}

			if (Delete)
				pExt->DeleteSelf();
		}
	}

	return 0;
}
