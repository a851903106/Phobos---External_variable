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

	if (!pThis || !pType || !pExt || !pExt->TypeExtData)
		return 0;

	if (pType->Passengers > 0 && pExt->TypeExtData->UseConvert.Get())
	{
		if (pExt->PassengerNum != pThis->Passengers.NumPassengers)
		{
			pExt->PassengerNum = pThis->Passengers.NumPassengers;

			if (pExt->PassengerNum >= 1)
			{
				pExt->LoadConvert();
			}
			else
			{
				pExt->UnloadConvert();
			}
		}
	}

	return 0;
}
