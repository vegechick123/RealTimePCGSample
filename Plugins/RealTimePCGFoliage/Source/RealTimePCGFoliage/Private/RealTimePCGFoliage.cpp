// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealTimePCGFoliage.h"
#include "RealTimePCGFoliageEdMode.h"

#define LOCTEXT_NAMESPACE "FRealTimePCGFoliageModule"

void FRealTimePCGFoliageModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FEditorModeRegistry::Get().RegisterMode<FRealTimePCGFoliageEdMode>(FRealTimePCGFoliageEdMode::EM_RealTimePCGFoliageEdModeId, LOCTEXT("RealTimePCGFoliageEdModeName", "RealTimePCGFoliageEdMode"), FSlateIcon(), true);
}

void FRealTimePCGFoliageModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FEditorModeRegistry::Get().UnregisterMode(FRealTimePCGFoliageEdMode::EM_RealTimePCGFoliageEdModeId);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRealTimePCGFoliageModule, RealTimePCGFoliage)