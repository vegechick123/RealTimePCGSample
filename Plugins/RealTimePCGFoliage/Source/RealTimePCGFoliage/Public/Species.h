// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FoliageType.h"
#include "Species.generated.h"

/**
 * 
 */
UCLASS()
class REALTIMEPCGFOLIAGE_API USpecies : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	TArray<UFoliageType*> FoliageTypes;
	UPROPERTY(EditAnywhere)
	UMaterialInterface* DensityCalculateMaterial;
	UPROPERTY(EditAnywhere)
	float MaxRadius;
	UPROPERTY(EditAnywhere)
	float MaxRandomScale = 1;
	UPROPERTY(EditAnywhere)
	float MinRandomScale = 1;
	UPROPERTY(EditAnywhere)
	float Ratio = 1;
};
