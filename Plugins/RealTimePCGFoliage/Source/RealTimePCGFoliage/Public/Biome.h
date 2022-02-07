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
USTRUCT(BlueprintType)
struct FBiomeData
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UTexture2D* PlacementMap;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<UTexture2D*> CleanMaps;
	UPROPERTY()
	FIntPoint TexSize;
	UPROPERTY()
	TWeakObjectPtr<UBiome> Biome;
	UPROPERTY(Transient,VisibleAnywhere, BlueprintReadOnly)
	TArray<UTextureRenderTarget2D*> DensityMaps;
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly)
	UTextureRenderTarget2D* InitPlacementMap;

	FBiomeData();
	FBiomeData(UObject* Outer, UBiome* InBiome, FIntPoint InTexSize);
	bool CheckBiome(UBiome* InBiome) const;
	void FillDensityMaps(FIntPoint InitPlacementMapResolution);
};