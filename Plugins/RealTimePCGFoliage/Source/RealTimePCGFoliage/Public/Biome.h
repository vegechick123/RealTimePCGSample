// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Species.h"
#include "Biome.generated.h"


UCLASS()
class REALTIMEPCGFOLIAGE_API UBiome : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	TArray<USpecies*> Species;
};
USTRUCT()
struct FBiomeData
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere)
	UTexture2D* PlacementMap;
	UPROPERTY(VisibleAnywhere)
	TArray<UTexture2D*> CleanMaps;
	UPROPERTY()
	FIntPoint TexSize;
	UPROPERTY()
	TWeakObjectPtr<UBiome> Biome;
	UPROPERTY(Transient,VisibleAnywhere)
	TArray<UTextureRenderTarget2D*> DensityMaps;
	FBiomeData();
	FBiomeData(UObject* Outer, UBiome* InBiome, FIntPoint InTexSize);
	bool CheckBiome(UBiome* InBiome) const;
	void FillDensityMaps();
};