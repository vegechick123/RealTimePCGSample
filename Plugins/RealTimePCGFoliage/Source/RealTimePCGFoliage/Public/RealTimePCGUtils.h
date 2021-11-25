// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
struct RealTimePCGUtils
{
	static void ReadDataToRenderTarget(UTextureRenderTarget2D* InRenderTarget, TArray<float> InData);

	static void SaveDataToArray(UTextureRenderTarget2D* InRenderTarget, TArray<float>& InData);

	static UTextureRenderTarget2D* GetOrCreateTransientRenderTarget2D(UTextureRenderTarget2D* InRenderTarget, FName InRenderTargetName, const FIntPoint& InSize, ETextureRenderTargetFormat InFormat, const FLinearColor& InClearColor, bool bInAutoGenerateMipMaps = false);
};
