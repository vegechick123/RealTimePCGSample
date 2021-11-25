// Fill out your copyright notice in the Description page of Project Settings.


#include "RealTimePCGUtils.h"

void RealTimePCGUtils::ReadDataToRenderTarget(UTextureRenderTarget2D* InRenderTarget, TArray<float> InData)
{

	const int32 SizeX = InRenderTarget->SizeX;
	const int32 SizeY = InRenderTarget->SizeY;

	FTextureRenderTargetResource* RTResource = InRenderTarget->GameThread_GetRenderTargetResource();
	ENQUEUE_RENDER_COMMAND(FReadDataToRenderTarget)(
		[RTResource, &InData, SizeX, SizeY](FRHICommandListImmediate& RHICmdList)
		{
			const FTexture2DRHIRef Texture2DRHI = RTResource->GetRenderTargetTexture();
			uint32 Stride;
			float* DestData = static_cast<float*>(RHILockTexture2D(Texture2DRHI, 0, RLM_WriteOnly, Stride, false));
			Stride = static_cast<uint32>(sizeof(float));
			FMemory::Memcpy(DestData, InData.GetData(), SizeX * SizeY * Stride);
			RHIUnlockTexture2D(Texture2DRHI, 0, false);
		});
	FlushRenderingCommands();
	InRenderTarget->UpdateResourceImmediate(false);
}

void RealTimePCGUtils::SaveDataToArray(UTextureRenderTarget2D* InRenderTarget, TArray<float>& InData)
{
	const int32 SizeX = InRenderTarget->SizeX;
	const int32 SizeY = InRenderTarget->SizeY;
	InData.AddUninitialized(SizeX * SizeY);
	FTextureRenderTargetResource* RTResource = InRenderTarget->GameThread_GetRenderTargetResource();
	ENQUEUE_RENDER_COMMAND(FReadDataFromRenderTarget)(
		[RTResource, &InData, SizeX, SizeY](FRHICommandListImmediate& RHICmdList)
		{
			const FTexture2DRHIRef Texture2DRHI = RTResource->GetRenderTargetTexture();
			uint32 Stride;
			float* DestData = static_cast<float*>(RHILockTexture2D(Texture2DRHI, 0, RLM_ReadOnly, Stride, false));
			Stride = static_cast<uint32>(sizeof(float));
			FMemory::Memcpy(InData.GetData(), DestData, SizeX * SizeY * Stride);
			RHIUnlockTexture2D(Texture2DRHI, 0, false);
		});
	FlushRenderingCommands();
	InRenderTarget->UpdateResourceImmediate(false);
}

UTextureRenderTarget2D* RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(UTextureRenderTarget2D* InRenderTarget, FName InRenderTargetName, const FIntPoint& InSize, ETextureRenderTargetFormat InFormat,
	const FLinearColor& InClearColor, bool bInAutoGenerateMipMaps)
{
	EPixelFormat PixelFormat = GetPixelFormatFromRenderTargetFormat(InFormat);
	if ((InSize.X <= 0)
		|| (InSize.Y <= 0)
		|| (PixelFormat == EPixelFormat::PF_Unknown))
	{
		return nullptr;
	}

	if (IsValid(InRenderTarget))
	{
		if ((InRenderTarget->SizeX == InSize.X)
			&& (InRenderTarget->SizeY == InSize.Y)
			&& (InRenderTarget->GetFormat() == PixelFormat) // Watch out : GetFormat() returns a EPixelFormat (non-class enum), so we can't compare with a ETextureRenderTargetFormat
			&& (InRenderTarget->ClearColor == InClearColor)
			&& (InRenderTarget->bAutoGenerateMips == bInAutoGenerateMipMaps))
		{
			return InRenderTarget;
		}
	}

	UTextureRenderTarget2D* NewRenderTarget2D = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UTextureRenderTarget2D::StaticClass(), InRenderTargetName));
	check(NewRenderTarget2D);
	NewRenderTarget2D->RenderTargetFormat = InFormat;
	NewRenderTarget2D->ClearColor = InClearColor;
	NewRenderTarget2D->bAutoGenerateMips = bInAutoGenerateMipMaps;
	NewRenderTarget2D->InitAutoFormat(InSize.X, InSize.Y);
	NewRenderTarget2D->UpdateResourceImmediate(true);

	// Flush RHI thread after creating texture render target to make sure that RHIUpdateTextureReference is executed before doing any rendering with it
	// This makes sure that Value->TextureReference.TextureReferenceRHI->GetReferencedTexture() is valid so that FUniformExpressionSet::FillUniformBuffer properly uses the texture for rendering, instead of using a fallback texture
	ENQUEUE_RENDER_COMMAND(FlushRHIThreadToUpdateTextureRenderTargetReference)(
		[](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		});

	return NewRenderTarget2D;
}