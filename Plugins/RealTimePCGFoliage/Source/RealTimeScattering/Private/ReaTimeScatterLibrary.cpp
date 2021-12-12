// Fill out your copyright notice in the Description page of Project Settings.


#include "ReaTimeScatterLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GPUScatteringCS.h"
#include "JumpFloodCS.h"
#include "UAVCleanCS.h"
#include "RenderGraphUtils.h"
#include "Async/ParallelFor.h"
#include "ClearQuad.h"
#include "KdTree.h"

void UReaTimeScatterLibrary::RealTImeScatter(const TArray<FColor>& ColorData, FIntPoint TextureSize, FVector2D BottomLeft, FVector2D TopRight, const FScatterPattern& Pattern, TArray<FVector2D>& Result, float RadiusScale, float Ratio, bool FlipY)
{
	TArray<TPair<bool, FVector2D>> MultithreadResult;
	FVector2D Size = TopRight - BottomLeft;
	FVector2D Temp = Size / (Pattern.Size * RadiusScale);
	int PerPointsCnt = Pattern.PointCloud.Num();
	FIntPoint CellNumber = FIntPoint(ceilf(Temp.X), ceilf(Temp.Y));
	int Total = CellNumber.X * CellNumber.Y;
	MultithreadResult.AddUninitialized(Total * Pattern.PointCloud.Num());
	ParallelFor(Total, [&](int index)
		{
			int i = index / CellNumber.X;
			int j = index % CellNumber.X;
			FVector2D Center = (FVector2D(j, i));//0~CellNumber
			for (int k = 0; k < Pattern.PointCloud.Num(); k++)
			{
				int ResultIndex = index * Pattern.PointCloud.Num() + k;
				MultithreadResult[ResultIndex].Key = false;
				FVector2D Pos = (Center * Pattern.Size + Pattern.PointCloud[k]) * RadiusScale;
				if (!(Pos.X < 0 || Pos.X >= Size.X || Pos.Y < 0 || Pos.Y >= Size.Y))
				{
					FVector2D uv = Pos / Size; //0...1
					if (FlipY)
						uv.Y=1-uv.Y;
					FVector2D Temp = FVector2D(uv.Y, uv.X) * TextureSize;
					
					
					FIntPoint TexturePos = FIntPoint(floor(Temp.X), floor(Temp.Y));

					float hash1 = FMath::Frac(FVector::DotProduct(FVector(k, j, j) * FVector(i, i, j) / 3.91, FVector(12.739, 3.1415926, 35.12)));
					float hash2 = FMath::Frac(FVector::DotProduct(FVector(i, j, k) * FVector(k, i, j) / 1.73, FVector(20.390625, 60.703125, 2.4281209)));
					int TextureIndex = TexturePos.X * TextureSize.Y + TexturePos.Y;
					if ((ColorData[TextureIndex].R * 1.0f) / 255 > hash1 && hash2 < Ratio)
					{
						MultithreadResult[ResultIndex].Key = true;
						MultithreadResult[ResultIndex].Value = Pos+BottomLeft;
					}
				}
			}
		}, false);
	for (TPair<bool, FVector2D> var : MultithreadResult)
	{
		if (var.Key)
		{
			Result.Add(var.Value);
		}
	};
	

	

}
void CleanUAV_RenderThread(FUnorderedAccessViewRHIRef UAV,float Value, ERHIFeatureLevel::Type FeatureLevel)
{
	/*TShaderMapRef<FUAVCleanCS>UAVCleanCS(GetGlobalShaderMap(FeatureLevel));
	FUAVCleanCS::FParameters Params;
	Params.Texture = UAV;
	Params.Value = Value;
	FComputeShaderUtils::Dispatch(RHICmdList, UAVCleanCS, Params, GroupCount);*/
}
static void RealTImeScatterGPU_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FRHITexture2D* DensityTexture,
	FRHITexture2D* PlacementSDFTexture,
	FRHITexture2D* InputSDFTexture,
	FUnorderedAccessViewRHIRef SDFSeedUAV,
	const FScatterPattern& Pattern,
	FVector4 TotalRect,
	FVector4 DirtyRect,
	float Ratio,
	float RadiusScale,
	bool FlipY,
	ERHIFeatureLevel::Type FeatureLevel,
	TArray<FScatterPoint>& OutputPointCloud
)
{

	check(IsInRenderingThread());
	FVector2D TotalSize = FVector2D(TotalRect.Z, TotalRect.W) - FVector2D(TotalRect.X, TotalRect.Y);

	FIntPoint TexSize = DensityTexture->GetSizeXY();
	FVector2D ScaledPatternSize = Pattern.Size * RadiusScale;
	FVector2D Temp = TotalSize / ScaledPatternSize;
	int PerPointsCnt = Pattern.PointCloud.Num();
	FIntPoint CellNumber = FIntPoint(ceilf(Temp.X), ceilf(Temp.Y));
	Temp = (FVector2D(DirtyRect.X, DirtyRect.Y) - FVector2D(TotalRect.X, TotalRect.Y)) / ScaledPatternSize;
	FIntPoint StartCell = FIntPoint(floorf(Temp.X), floorf(Temp.Y));
	Temp =  (FVector2D(DirtyRect.Z, DirtyRect.W) - FVector2D(TotalRect.X, TotalRect.Y)) / ScaledPatternSize;
	FIntPoint EndCell = FIntPoint(ceilf(Temp.X), ceilf(Temp.Y));

	FIntPoint DirtyCellNumber = EndCell - StartCell ;
	int Total = CellNumber.X * CellNumber.Y;


	TShaderMapRef<FGPUScatteringCS>GPUScatteringCS(GetGlobalShaderMap(FeatureLevel));
	FGPUScatteringCS::FParameters Params;



	TResourceArray<uint32> bufferData;
	bufferData.Reset(1);
	bufferData.Add(0);
	bufferData.SetAllowCPUAccess(true);

	FRHIResourceCreateInfo CreateInfo;
	CreateInfo.ResourceArray = &bufferData;
		
	FStructuredBufferRHIRef  InstanceCountBuffer = RHICreateStructuredBuffer(sizeof(uint32), sizeof(uint32) * 1, BUF_UnorderedAccess | BUF_ShaderResource, CreateInfo);
	FUnorderedAccessViewRHIRef InstanceCountBufferUAV = RHICreateUnorderedAccessView(InstanceCountBuffer, true, false);
	Params.InstanceCountBuffer = InstanceCountBufferUAV;

	for (int i = 0; i < Pattern.PointCloud.Num(); i++)
	{
		Params.InputPattern[i] = Pattern.PointCloud[i];
	}

	Params.PatternSize = Pattern.Size;
	Params.PatternPointNum = Pattern.PointCloud.Num();
	Params.InputTexture = DensityTexture;
	Params.PlacementSDF = PlacementSDFTexture;
	Params.InputSDF = InputSDFTexture;
	Params.OutputSDF = SDFSeedUAV;
	Params.LinearSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
	Params.CenterOffset = StartCell;
	Params.TotalRect = TotalRect;
	Params.ClipRect = DirtyRect;
	Params.Ratio = Ratio;
	Params.RadiusScale = RadiusScale;
	Params.FlipY = FlipY ? 1 : 0;

	TResourceArray<FScatterPoint> OutputBufferData;
	const int Maxnum = Total * Params.PatternPointNum;
	OutputBufferData.Reset(Maxnum);
	OutputBufferData.AddUninitialized(Maxnum);
	OutputBufferData.SetAllowCPUAccess(true);
	CreateInfo.ResourceArray = &OutputBufferData;
	FStructuredBufferRHIRef  OutputBuffer = RHICreateStructuredBuffer(sizeof(FScatterPoint), sizeof(FScatterPoint) * Maxnum, BUF_UnorderedAccess | BUF_ShaderResource, CreateInfo);
	FUnorderedAccessViewRHIRef OutputBufferUAV = RHICreateUnorderedAccessView(OutputBuffer, true, false);
	Params.OutputPointCloud = OutputBufferUAV;

	FComputeShaderUtils::Dispatch(RHICmdList, GPUScatteringCS, Params, FIntVector(DirtyCellNumber.X, DirtyCellNumber.Y, 1));

	double start = FPlatformTime::Seconds();

	uint32 Count = *(uint32*)RHILockStructuredBuffer(InstanceCountBuffer, 0, sizeof(uint32), EResourceLockMode::RLM_ReadOnly);
	
	double end1 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("ReadBackCount executed in %f seconds."), end1 - start);
	       
	RHIUnlockStructuredBuffer(InstanceCountBuffer.GetReference());
	if (Count != 0)
	{
		//TODO
		TArray<FScatterPoint>& OutputData = OutputPointCloud;
		OutputData.Reset(Count);
		OutputData.AddUninitialized(Count);
		FScatterPoint* srcptr = (FScatterPoint*)RHILockStructuredBuffer(OutputBuffer, 0, sizeof(FScatterPoint) * Maxnum, EResourceLockMode::RLM_ReadOnly);
		//
		FMemory::Memcpy(OutputData.GetData(), srcptr, sizeof(FScatterPoint) * Count);
		RHIUnlockStructuredBuffer(OutputBuffer.GetReference());
		//
	}
	double end2 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("ReadBackPointCloud executed in %f seconds."), end2 - end1);
	
}
static void JumpFlood_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FRHITexture2D* InputSeedTexture,
	FUnorderedAccessViewRHIRef OutputSDFRTUAV,
	bool SubstractSeedRadius,
	float LengthScale,
	ERHIFeatureLevel::Type FeatureLevel)
{
	TShaderMapRef<FJFAInitCS>JFAInitCS(GetGlobalShaderMap(FeatureLevel));
	FJFAInitCS::FParameters JFAInitParams;
	TShaderMapRef<FJFAStepCS>JFAStepCS(GetGlobalShaderMap(FeatureLevel));
	FJFAStepCS::FParameters JFAStepParams;
	TShaderMapRef<FJFASDFOutputCS>JFASDFOutputCS(GetGlobalShaderMap(FeatureLevel));
	FJFASDFOutputCS::FParameters JFASDFOutputParams;

	FIntPoint TexSize = InputSeedTexture->GetSizeXY();

	FRHIResourceCreateInfo CreateInfo;
	FTexture2DRHIRef PingPointRT1 = RHICreateTexture2D(TexSize.X, TexSize.Y, PF_R32G32_UINT, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef PingPointRT1UAV = RHICreateUnorderedAccessView(PingPointRT1);
	FTexture2DRHIRef PingPointRT2 = RHICreateTexture2D(TexSize.X, TexSize.Y, PF_R32G32_UINT, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef PingPointRT2UAV = RHICreateUnorderedAccessView(PingPointRT2);
	
	//JFAInit
	JFAInitParams.InputSeed = InputSeedTexture;
	JFAInitParams.OutputStepRT = PingPointRT1UAV;
	JFAInitParams.Inverse = SubstractSeedRadius ? 0 : 1;
	FIntVector GroupCount;
	GroupCount.X = (TexSize.X - 1) / 8 + 1;
	GroupCount.Y = (TexSize.Y - 1) / 8 + 1;
	GroupCount.Z = 1;

	//FGPUFenceRHIRef Fence = RHICmdList.CreateGPUFence(FName("111"));
	//JFAInitCS->Fence
	FComputeShaderUtils::Dispatch(RHICmdList, JFAInitCS, JFAInitParams, GroupCount);
	//Debug


	//JFAStep
	uint32 Step = FMath::Max(PingPointRT1->GetSizeX(), PingPointRT1->GetSizeY()) / 2;
	bool PingPong = false;
	JFAStepParams.InputSeed = InputSeedTexture;
	JFAStepParams.SubstractSeedRadius = SubstractSeedRadius ? 1 : 0;
	JFAStepParams.LengthScale = LengthScale;
	while (Step >= 1)
	{
		if (!PingPong)
		{
			JFAStepParams.InputStepRT = PingPointRT1;
			JFAStepParams.OutputStepRT = PingPointRT2UAV;
		}
		else
		{
			JFAStepParams.InputStepRT = PingPointRT2;
			JFAStepParams.OutputStepRT = PingPointRT1UAV;
		}
		JFAStepParams.Step = Step;
		PingPong = !PingPong;
		Step >>= 1;
		FComputeShaderUtils::Dispatch(RHICmdList, JFAStepCS, JFAStepParams, GroupCount);


	}

	////JFAOutputSDF
	JFASDFOutputParams.InputSeed = InputSeedTexture;
	if (!PingPong)
		JFASDFOutputParams.InputStepRT = PingPointRT1;
	else
		JFASDFOutputParams.InputStepRT = PingPointRT2;
	JFASDFOutputParams.OutputSDF = OutputSDFRTUAV;
	JFASDFOutputParams.SubstractSeedRadius = SubstractSeedRadius?1:0;
	JFASDFOutputParams.LengthScale = LengthScale;

	FComputeShaderUtils::Dispatch(RHICmdList, JFASDFOutputCS, JFASDFOutputParams, GroupCount);

	
}

void ScatterFoliagePass_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* PlacementResources,
	TArray<FTextureRenderTargetResource*> DensityResources,
	FTextureRenderTargetResource* SDFResource,
	const FScatterPattern& Pattern,
	FVector4 TotalRect,
	FVector4 DirtyRect,
	const TArray<FSpeciesProxy>& InData,
	bool FlipY,
	ERHIFeatureLevel::Type FeatureLevel,
	TArray<FScatterPointCloud>& OutputPointCloud
)
{
	check(IsInRenderingThread());

	
	FRHITexture2D* SDFTexture = SDFResource->GetRenderTargetTexture();
	

	FIntPoint TexSize = SDFTexture->GetSizeXY();

	FRHIResourceCreateInfo CreateInfo;
	FTexture2DRHIRef SDFSeedTexture = RHICreateTexture2D(TexSize.X, TexSize.Y, PF_R32_FLOAT, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef SDFSeedTextureUAV = RHICreateUnorderedAccessView(SDFSeedTexture);
	FTexture2DRHIRef OutputSDFRT = RHICreateTexture2D(TexSize.X, TexSize.Y, PF_R32_FLOAT, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef OutputSDFRTUAV = RHICreateUnorderedAccessView(OutputSDFRT);
	
	FTexture2DRHIRef PlacementSDFRT = RHICreateTexture2D(TexSize.X, TexSize.Y, PF_R32_FLOAT, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef PlacementSDFRTUAV = RHICreateUnorderedAccessView(PlacementSDFRT);

	FIntVector GroupCount= FIntVector(TexSize.X/8, TexSize.Y / 8,1);
	TShaderMapRef<FUAVCleanCS>UAVCleanCS(GetGlobalShaderMap(FeatureLevel));
	FUAVCleanCS::FParameters Params;
	Params.Texture = OutputSDFRTUAV;
	Params.Value = 1e16;
	FComputeShaderUtils::Dispatch(RHICmdList, UAVCleanCS, Params, GroupCount);
	Params.Texture = SDFSeedTextureUAV;
	Params.Value = 0;
	FComputeShaderUtils::Dispatch(RHICmdList, UAVCleanCS, Params, GroupCount);

	Params.Texture = PlacementSDFRTUAV;
	Params.Value = 1e16;
	FComputeShaderUtils::Dispatch(RHICmdList, UAVCleanCS, Params, GroupCount);

	float LengthScale = TotalRect.Z - TotalRect.X;

	JumpFlood_RenderThread
	(
		RHICmdList,
		PlacementResources->GetRenderTargetTexture(),
		PlacementSDFRTUAV,
		false,
		LengthScale,
		FeatureLevel
	);
	for (int i = 0; i < InData.Num(); i++)
	{
	
		//Generate Point Cloud Data
		float Ratio=InData[i].Ratio;
		float RadiusScale= InData[i].Radius;
		RealTImeScatterGPU_RenderThread
		(
			RHICmdList,
			DensityResources[i]->GetRenderTargetTexture(),
			PlacementSDFRT,
			OutputSDFRT,
			SDFSeedTextureUAV,
			Pattern,
			TotalRect,
			DirtyRect,
			Ratio,
			RadiusScale,
			FlipY,
			FeatureLevel,
			OutputPointCloud[i].ScatterPoints
		);
		//Generate Collision SDF
		JumpFlood_RenderThread
		(
			RHICmdList,
			SDFSeedTexture,
			OutputSDFRTUAV,
			true,
			LengthScale,
			FeatureLevel
		);
	}

	FRHICopyTextureInfo CopyInfo;
	CopyInfo.Size.X = TexSize.X;
	CopyInfo.Size.Y = TexSize.Y;
	CopyInfo.Size.Z = 1;
	RHICmdList.CopyTexture(PlacementSDFRT, SDFTexture, CopyInfo);
	
}
void UReaTimeScatterLibrary::RealTImeScatterGPU(
	UObject* WorldContextObject,
	UTextureRenderTarget2D* PlacementMap,
	TArray<UTextureRenderTarget2D*> DensityMaps,
	UTextureRenderTarget2D* OutputDistanceField, 
	FVector4 TotalRect, 
	FVector4 DirtyRect,
	const FScatterPattern& Pattern,
	const TArray<FSpeciesProxy>& InData,
	TArray<FScatterPointCloud>& Result, 
	bool FlipY)
{
	check(IsInGameThread());

	FTextureRenderTargetResource* PlacementMapResource = PlacementMap->GameThread_GetRenderTargetResource();

	TArray<FTextureRenderTargetResource*> DensityResources;
	for(UTextureRenderTarget2D* DensityMap:DensityMaps)
		DensityResources.Add(DensityMap->GameThread_GetRenderTargetResource());
	
	FTextureRenderTargetResource* OutputDistanceFieldResource = OutputDistanceField->GameThread_GetRenderTargetResource();

	//PlacementMap->Resource->GetTexture2DResource
	ERHIFeatureLevel::Type FeatureLevel = WorldContextObject->GetWorld()->Scene->GetFeatureLevel();
	

	ENQUEUE_RENDER_COMMAND(CaptureCommand)
		(
			[PlacementMapResource, DensityResources, OutputDistanceFieldResource, FeatureLevel,&Pattern, TotalRect, DirtyRect, InData,FlipY,&Result](FRHICommandListImmediate& RHICmdList)
			{
				ScatterFoliagePass_RenderThread
				(
					RHICmdList,
					PlacementMapResource,
					DensityResources,					
					OutputDistanceFieldResource,
					Pattern,
					TotalRect,
					DirtyRect,
					InData,
					FlipY,
					FeatureLevel,
					Result
				);
			}
	);

	FRenderCommandFence Fence;
	Fence.BeginFence();
	Fence.Wait();
}

void UReaTimeScatterLibrary::ScatterWithCollision(
	UObject* WorldContextObject,
	TArray<UTextureRenderTarget2D*> DensityMaps,
	UTextureRenderTarget2D* OutputDistanceField, 
	FVector2D BottomLeft, FVector2D TopRight, 
	const FScatterPattern& Pattern, 
	const TArray<FSpeciesProxy>& InData, 
	TArray<FScatterPointCloud>& OutData, 
	bool FlipY,
	bool UseGPU)
{
	return;
	/*if (!Mask||InData.Num() == 0)
		return;
	TArray<FColor> Samples;
	FIntRect SampleRect(0, 0, Mask->SizeX, Mask->SizeY);

	FRenderCommandFence Fence;
	OutData.AddDefaulted(InData.Num());
	RealTImeScatterGPU(WorldContextObject, Mask, OutputDistanceField, BottomLeft, TopRight, Pattern, InData, OutData,  FlipY);
	Fence.BeginFence();
	Fence.Wait();*/

}


void UReaTimeScatterLibrary::JumpFlood(
	UObject* WorldContextObject,
	UTextureRenderTarget2D* InputSeed, 
	UTextureRenderTarget2D* InputStepRT, 
	UTextureRenderTarget2D* OutputStepRT,
	UTextureRenderTarget2D* OutputSDF,
	float LengthScale)
{
	check(IsInGameThread());


	/*FRHITexture* InputSeedResource = InputSeed->TextureReference.TextureReferenceRHI->GetTextureReference()->GetReferencedTexture();
	FTextureRenderTargetResource* InputStepRTResource = InputStepRT->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* OutputStepRTResource = OutputStepRT->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* OutputSDFResource = OutputSDF->GameThread_GetRenderTargetResource();
	ERHIFeatureLevel::Type FeatureLevel = WorldContextObject->GetWorld()->Scene->GetFeatureLevel();
	ENQUEUE_RENDER_COMMAND(CaptureCommand)
	(
		[InputSeedResource, InputStepRTResource, OutputStepRTResource, OutputSDFResource, LengthScale, FeatureLevel](FRHICommandListImmediate& RHICmdList)
		{
			JumpFlood_RenderThread
			(
				RHICmdList,
				InputSeedResource,
				InputStepRTResource,
				OutputStepRTResource,
				OutputSDFResource,
				LengthScale,
				FeatureLevel
			);
		}
	);*/
}
