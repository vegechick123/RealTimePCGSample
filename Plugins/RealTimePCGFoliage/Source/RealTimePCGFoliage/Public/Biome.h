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
struct FRenderTargetData
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient, VisibleAnywhere)
	UTextureRenderTarget2D* RenderTarget;
	UPROPERTY()
	TArray<float> Data;
	FRenderTargetData();
	void SaveRenderTargetData();
	void LoadDataToRenderTarget();
};
USTRUCT()
struct FBiomeData
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere)
	FRenderTargetData PlacementRTData;
	UPROPERTY()
	TArray<FRenderTargetData> CleanRTData;
	FBiomeData();
	void SaveRenderTargetData();
	void CreateRenderTarget(FIntPoint TexSize);
	void LoadRenderTargetData();
};