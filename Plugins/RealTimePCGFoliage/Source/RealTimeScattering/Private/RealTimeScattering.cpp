// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealTimeScattering.h"
#include "Interfaces/IPluginManager.h"
#define LOCTEXT_NAMESPACE "FRealTimeScatteringModule"

void FRealTimeScatteringModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("RealTimePCGFoliage"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping("/CustomShaders", PluginShaderDir);
}

void FRealTimeScatteringModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRealTimeScatteringModule, RealTimeScattering)