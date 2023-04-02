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
		Valueable<bool> ExVartoVar;
		PhobosFixedString<32U> ExVartoVar_File;
		PhobosFixedString<32U> ExVartoVar_Section;
		PhobosFixedString<32U> ExVartoVar_Key;
		std::vector<SuperWeaponTypeClass*> ExVartoVar_SW;

		Valueable<bool> VartoExVar;
		PhobosFixedString<32U> VartoExVar_File;
		PhobosFixedString<32U> VartoExVar_Section;
		PhobosFixedString<32U> VartoExVar_Key;
		PhobosFixedString<32U> VartoExVar_Value;

		ExtData(TechnoTypeClass* OwnerObject) : Extension<TechnoTypeClass>(OwnerObject)
			, ExVartoVar { false }
			, ExVartoVar_File { nullptr }
			, ExVartoVar_Section { nullptr }
			, ExVartoVar_Key { nullptr }
			, ExVartoVar_SW {}

			, VartoExVar { false }
			, VartoExVar_File { nullptr }
			, VartoExVar_Section { nullptr }
			, VartoExVar_Key { nullptr }
			, VartoExVar_Value { nullptr }
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
