// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeciesFactory.h"

USpeciesFactory::USpeciesFactory()
{
    bCreateNew = true;
    bEditAfterNew = true;
    SupportedClass = USpecies::StaticClass();
}

UObject* USpeciesFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
    return NewObject<USpecies>(InParent,InClass,InName,Flags);
}
