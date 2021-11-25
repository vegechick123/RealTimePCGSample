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


