// Fill out your copyright notice in the Description page of Project Settings.


#include "Biome.h"
#include "RealTimePCGUtils.h"

FRenderTargetData::FRenderTargetData()
{
}

void FRenderTargetData::SaveRenderTargetData()
{
	if (RenderTarget != nullptr)
	{
		Data.Empty();
		RealTimePCGUtils::SaveDataToArray(RenderTarget, Data);
	}
}

void FRenderTargetData::LoadDataToRenderTarget()
{
	if (RenderTarget != nullptr)
	{
		FIntPoint TexSize = FIntPoint(RenderTarget->SizeX, RenderTarget->SizeY);
		if(TexSize.X*TexSize.Y==Data.Num())
		RealTimePCGUtils::ReadDataToRenderTarget(RenderTarget, Data);
	}
}

FBiomeData::FBiomeData()
{
}

void FBiomeData::SaveRenderTargetData()
{
	PlacementRTData.SaveRenderTargetData();
	for (FRenderTargetData& RenderTargetData : CleanRTData)
	{
		RenderTargetData.SaveRenderTargetData();
	}
}

void FBiomeData::CreateRenderTarget(FIntPoint TexSize)
{
	PlacementRTData.RenderTarget=RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(PlacementRTData.RenderTarget,"PlacementRT",TexSize,ETextureRenderTargetFormat::RTF_R32f,FLinearColor::Black);
	for (FRenderTargetData& RenderTargetData : CleanRTData)
	{
		RenderTargetData.RenderTarget = RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(RenderTargetData.RenderTarget, "CleanRT", TexSize, ETextureRenderTargetFormat::RTF_R32f, FLinearColor::Black);
	}
}

void FBiomeData::LoadRenderTargetData()
{
	PlacementRTData.LoadDataToRenderTarget();
	for (FRenderTargetData& RenderTargetData : CleanRTData)
	{
		RenderTargetData.LoadDataToRenderTarget();
	}
}

FBiomeRenderTargetData::FBiomeRenderTargetData()
{
}

FBiomeRenderTargetData::FBiomeRenderTargetData(UObject* Outer,UBiome* Biome, FIntPoint TexSize)
{
	//getName
	auto CreateRenderTarget = [TexSize,Outer](FName Name)
	{
		UTexture2D* NewTexture = NewObject<UTexture2D>(Outer, MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), Name));
		

		NewTexture->Source.Init(TexSize.X, TexSize.Y, 1, 1, ETextureSourceFormat::TSF_G8);
		uint8* TextureData = NewTexture->Source.LockMip(0);
		FMemory::Memzero(TextureData, sizeof(uint8) * TexSize.X * TexSize.Y);
		NewTexture->Source.UnlockMip(0);
		NewTexture->SRGB = false;
		NewTexture->CompressionNone = true;
		NewTexture->DeferCompression = false;
		NewTexture->MipGenSettings = TMGS_NoMipmaps;
		NewTexture->PostEditChange();
		check(NewTexture);
		return NewTexture;
	};

	PlacementRT = CreateRenderTarget(FName(Biome->GetName() + "Placement"));
	for (int i = 0; i < Biome->Species.Num(); i++)
		CleanRTs.Add(CreateRenderTarget(FName(Biome->GetName() + "CleanRT" + FString::FromInt(i))));


}
