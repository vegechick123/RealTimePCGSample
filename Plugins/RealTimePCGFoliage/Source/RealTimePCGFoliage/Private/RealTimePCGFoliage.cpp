// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealTimePCGFoliage.h"
#include "RealTimePCGFoliageEdMode.h"
#include "RealTimePCGAssetTypeActions.h"
#include "PCGFoliageEditorCommands.h"
#define LOCTEXT_NAMESPACE "FRealTimePCGFoliageModule"

EAssetTypeCategories::Type FRealTimePCGFoliageModule::AssetCategory;

void FRealTimePCGFoliageModule::RegisterAssetTypeActionIntoCategories(IAssetTools& AssetTools,TSharedRef<IAssetTypeActions> AssetTypeActions)
{
	AssetTools.RegisterAssetTypeActions(AssetTypeActions);
	CreatedAssetTypeActions.Add(AssetTypeActions);
}

void FRealTimePCGFoliageModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FPCGFoliageEditorCommands::Register();
	FEditorModeRegistry::Get().RegisterMode<FRealTimePCGFoliageEdMode>(FRealTimePCGFoliageEdMode::EM_RealTimePCGFoliageEdModeId, LOCTEXT("RealTimePCGFoliageEdModeName", "RealTimePCGFoliageEdMode"), FSlateIcon(), true);

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("PCGFoliage")), LOCTEXT("RealTimePCGAssetCategory", "Real Time ProcedualFoliage"));
	
	RegisterAssetTypeActionIntoCategories(AssetTools, MakeShareable(new FAssetTypeActions_Species));
	RegisterAssetTypeActionIntoCategories(AssetTools, MakeShareable(new FAssetTypeActions_Biome));
}

void FRealTimePCGFoliageModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FEditorModeRegistry::Get().UnregisterMode(FRealTimePCGFoliageEdMode::EM_RealTimePCGFoliageEdModeId);
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (TSharedRef<IAssetTypeActions> Action:CreatedAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action);
		}
	}
}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRealTimePCGFoliageModule, RealTimePCGFoliage)