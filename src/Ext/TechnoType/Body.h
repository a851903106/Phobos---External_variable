#pragma once
#include <TechnoTypeClass.h>
#include <SuperWeaponTypeClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

class Matrix3D;

class TechnoTypeExt
{
public:
	using base_type = TechnoTypeClass;

	class ExtData final : public Extension<TechnoTypeClass>
	{
	public:
		Valueable<bool> UseConvert;
		Nullable<UnitTypeClass*> Convert_Load;
		Nullable<UnitTypeClass*> Convert_Unload;
		std::vector<NullableVector<TechnoTypeClass*>> Convert_Loads;
		std::vector<UnitTypeClass*> Convert_Types;

		ExtData(TechnoTypeClass* OwnerObject) : Extension<TechnoTypeClass>(OwnerObject)
			, UseConvert { false }
			, Convert_Load {}
			, Convert_Unload {}
			, Convert_Loads {}
			, Convert_Types {}
		{ }

		virtual ~ExtData() = default;
		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void Initialize() override;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TechnoTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
