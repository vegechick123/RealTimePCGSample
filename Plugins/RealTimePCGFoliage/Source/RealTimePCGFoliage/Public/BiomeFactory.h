// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Biome.h"
#include "BiomeFactory.generated.h"

/**
 * 
 */
UCLASS()
class REALTIMEPCGFOLIAGE_API UBiomeFactory : public UFactory
{
	GENERATED_BODY()
	UBiomeFactory();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
};
