#pragma once

#include <Utilities/Container.h>
#include <Utilities/Constructs.h>
#include <Utilities/Template.h>

#include <Helpers/Template.h>

#include <TActionClass.h>

class HouseClass;

enum class PhobosTriggerAction : unsigned int
{
	TechnoUninit = 40000,
	MessageForSpecifiedColor = 40001,

	FireSuperWeapon = 50000,
	CreateAnimation = 50001,
	DeleteAnimation = 50002,
	SelectBox = 50003,
	SetSuperWeaponTimer = 50004
};

class TActionExt
{
public:
	using base_type = TActionClass;

	class ExtData final : public Extension<TActionClass>
	{
	public:
		ExtData(TActionClass* const OwnerObject) : Extension<TActionClass>(OwnerObject)
		{ }

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	static bool Execute(TActionClass* pThis, HouseClass* pHouse,
			ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location, bool& bHandled);

#pragma push_macro("ACTION_FUNC")
#define ACTION_FUNC(name) \
	static bool name(TActionClass* pThis, HouseClass* pHouse, \
		ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)

	ACTION_FUNC(TechnoUninit);
	ACTION_FUNC(MessageForSpecifiedColor);

	ACTION_FUNC(FireSuperWeapon);
	ACTION_FUNC(CreateAnimation);
	ACTION_FUNC(DeleteAnimation);
	ACTION_FUNC(SelectBox);
	ACTION_FUNC(SetSuperWeaponTimer);

	static void ClearManager(TechnoClass* pThis);

#undef ACTION_FUNC
#pragma pop_macro("ACTION_FUNC")

	class ExtContainer final : public Container<TActionExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
