// Fill out your copyright notice in the Description page of Project Settings.


#include "RealTimePCGAssetTypeActions.h"
FAssetTypeActions_Species::FAssetTypeActions_Species()
{
}

FText FAssetTypeActions_Species::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Species","Species");
}

UClass* FAssetTypeActions_Species::GetSupportedClass() const
{
	return USpecies::StaticClass();
}

FColor FAssetTypeActions_Species::GetTypeColor() const
{
	return FColor::Purple;
}

uint32 FAssetTypeActions_Species::GetCategories()
{
	return FRealTimePCGFoliageModule::AssetCategory;
}

FAssetTypeActions_Biome::FAssetTypeActions_Biome()
{
}

FText FAssetTypeActions_Biome::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Biome", "Biome");
}

UClass* FAssetTypeActions_Biome::GetSupportedClass() const
{
	return UBiome::StaticClass();
}

FColor FAssetTypeActions_Biome::GetTypeColor() const
{
	return FColor::Purple;
}

uint32 FAssetTypeActions_Biome::GetCategories()
{
	return FRealTimePCGFoliageModule::AssetCategory;
}
