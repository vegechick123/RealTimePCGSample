// Fill out your copyright notice in the Description page of Project Settings.


#include "Biome.h"
#include "RealTimePCGUtils.h"

FBiomeData::FBiomeData()
{
}

FBiomeData::FBiomeData(UObject* Outer,UBiome* InBiome, FIntPoint InTexSize)
{
	//getName
	TexSize = InTexSize;
	Biome = InBiome;
	auto CreateTexture = [this,Outer](FName Name)
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

	PlacementMap = CreateTexture(FName(Biome->GetName() + "Placement"));
	for (int i = 0; i < Biome->Species.Num(); i++)
		CleanMaps.Add(CreateTexture(FName(Biome->GetName() + "CleanRT" + FString::FromInt(i))));


}

bool FBiomeData::CheckBiome(UBiome* InBiome)const
{
	if (!PlacementMap)
		return false;
	if(!Biome.IsValid()||Biome.Get()!=InBiome)
		return false;
	if (Biome->Species.Num() != CleanMaps.Num())
		return false;
	return true;
}

void FBiomeData::FillDensityMaps()
{


	for (int i = DensityMaps.Num(); i < CleanMaps.Num(); i++)
	{
		DensityMaps.Add(RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(nullptr, "DensityMap", TexSize,ETextureRenderTargetFormat::RTF_R32f,FLinearColor::Black));
	}
}
