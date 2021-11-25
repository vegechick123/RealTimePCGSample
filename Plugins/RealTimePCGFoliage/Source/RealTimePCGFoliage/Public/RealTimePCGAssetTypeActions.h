// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "Species.h"
#include "Biome.h"
#include "RealTimePCGFoliage.h"
/**
 * 
 */

class FAssetTypeActions_Species: public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_Species();
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
};

class FAssetTypeActions_Biome : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_Biome();
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
};