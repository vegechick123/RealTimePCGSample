// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Biome.h"
#include "RealTimePCGAssetTypeActions.h"
#include "AssetToolsModule.h"

class FRealTimePCGFoliageModule : public IModuleInterface
{
public:

	

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void RegisterAssetTypeActionIntoCategories(IAssetTools& AssetTools,TSharedRef<IAssetTypeActions> AssetTypeActions);
	TArray<TSharedRef<IAssetTypeActions>> CreatedAssetTypeActions;
	static EAssetTypeCategories::Type AssetCategory;
};
