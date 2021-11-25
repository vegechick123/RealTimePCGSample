// Fill out your copyright notice in the Description page of Project Settings.


#include "BiomeFactory.h"

UBiomeFactory::UBiomeFactory()
{
    bCreateNew = true;
    bEditAfterNew = true;
    SupportedClass = UBiome::StaticClass();
}

UObject* UBiomeFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
    return NewObject<UBiome>(InParent, InClass, InName, Flags);
}
