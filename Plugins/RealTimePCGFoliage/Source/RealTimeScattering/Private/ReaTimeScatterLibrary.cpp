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
void FPointCloudReadBackBuffer::InitBuffer(int32 InMaxNum)
{
	MaxNum = InMaxNum;
	TResourceArray<uint32> InstanceBufferData;
	InstanceBufferData.Reset(1);
	InstanceBufferData.Add(0);
	InstanceBufferData.SetAllowCPUAccess(true);
		
	FRHIResourceCreateInfo CreateInfo;
	CreateInfo.ResourceArray = &InstanceBufferData;
		
	InstanceCountBuffer = RHICreateStructuredBuffer(sizeof(uint32), sizeof(uint32) * 1, BUF_UnorderedAccess | BUF_ShaderResource, CreateInfo);
		
	OutputBufferData.Reset(MaxNum);
	OutputBufferData.AddUninitialized(MaxNum);
	OutputBufferData.SetAllowCPUAccess(true);
	CreateInfo.ResourceArray = &OutputBufferData;
	OutputBuffer = RHICreateStructuredBuffer(sizeof(FScatterPoint), sizeof(FScatterPoint) * MaxNum, BUF_UnorderedAccess | BUF_ShaderResource, CreateInfo);		
}
void FPointCloudReadBackBuffer::ReadBackToArray(TArray<FScatterPoint>& OutputData)
{
	double start = FPlatformTime::Seconds();		
	uint32 Count = *(uint32*)RHILockStructuredBuffer(InstanceCountBuffer, 0, sizeof(uint32), EResourceLockMode::RLM_ReadOnly);
	RHIUnlockStructuredBuffer(InstanceCountBuffer.GetReference());
	FScatterPoint* Srcptr = (FScatterPoint*)RHILockStructuredBuffer(OutputBuffer, 0, sizeof(FScatterPoint) * MaxNum, EResourceLockMode::RLM_ReadOnly);
	OutputData.Reset(Count);
	OutputData.AddUninitialized(Count);
	FMemory::Memcpy(OutputData.GetData(), Srcptr, sizeof(FScatterPoint) * Count);
	RHIUnlockStructuredBuffer(OutputBuffer.GetReference());

	double end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("ReadBack executed in %f seconds."), end - start);
}
void FBiomePipelineContext::InitRenderThreadResource()
{
	PlacementMapResource = PlacementMap->GameThread_GetRenderTargetResource();

	DepthMapResource = DepthMap->GameThread_GetRenderTargetResource();
	for (UTextureRenderTarget2D* DensityMap : DensityMaps)
		DensityResources.Add(DensityMap->GameThread_GetRenderTargetResource());

	OutputDistanceFieldResource = OutputDistanceField->GameThread_GetRenderTargetResource();
	
}
static void RealTImeScatterGPU_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FRHITexture2D* DensityTexture,
	FRHITexture2D* DepthTexture,
	FRHITexture2D* PlacementSDFTexture,
	FRHITexture2D* InputSDFTexture,
	FUnorderedAccessViewRHIRef SDFSeedUAV,
	const FScatterPattern& Pattern,
	float BasicHeight,
	FVector4 TotalRect,
	FVector4 SimulateRect,
	FVector4 DirtyRect,
	float Ratio,
	float RadiusScale,
	bool FlipY,
	ERHIFeatureLevel::Type FeatureLevel,
	FPointCloudReadBackBuffer& PointCloudReadBackBuffer
)
{

	check(IsInRenderingThread());
	FVector2D TotalSize = FVector2D(TotalRect.Z, TotalRect.W) - FVector2D(TotalRect.X, TotalRect.Y);

	FIntPoint TexSize = DensityTexture->GetSizeXY();
	FVector2D ScaledPatternSize = Pattern.Size * RadiusScale;
	FVector2D Temp = TotalSize / ScaledPatternSize;
	int PerPointsCnt = Pattern.PointCloud.Num();
	FIntPoint CellNumber = FIntPoint(ceilf(Temp.X), ceilf(Temp.Y));

	
	Temp = (FVector2D(SimulateRect.X, SimulateRect.Y) - FVector2D(TotalRect.X, TotalRect.Y)) / ScaledPatternSize;
	FIntPoint StartCell = FIntPoint(floorf(Temp.X), floorf(Temp.Y));
	
	
	
	Temp =  (FVector2D(SimulateRect.Z, SimulateRect.W) - FVector2D(TotalRect.X, TotalRect.Y)) / ScaledPatternSize;
	FIntPoint EndCell = FIntPoint(ceilf(Temp.X), ceilf(Temp.Y));

	StartCell.X = FMath::Max(0, StartCell.X);
	StartCell.Y = FMath::Max(0, StartCell.Y);
	EndCell.X = FMath::Min(CellNumber.X, EndCell.X);
	EndCell.Y = FMath::Min(CellNumber.Y, EndCell.Y);

	FIntPoint DirtyCellNumber = EndCell - StartCell ;
	int TotalCellNumber = CellNumber.X * CellNumber.Y;


	TShaderMapRef<FGPUScatteringCS>GPUScatteringCS(GetGlobalShaderMap(FeatureLevel));
	FGPUScatteringCS::FParameters Params;

	const int Maxnum = TotalCellNumber * Pattern.PointCloud.Num();

	PointCloudReadBackBuffer.InitBuffer(Maxnum);

	FUnorderedAccessViewRHIRef InstanceCountBufferUAV = RHICreateUnorderedAccessView(PointCloudReadBackBuffer.InstanceCountBuffer, true, false);
	FUnorderedAccessViewRHIRef OutputBufferUAV = RHICreateUnorderedAccessView(PointCloudReadBackBuffer.OutputBuffer, true, false);
	Params.InstanceCountBuffer = InstanceCountBufferUAV;


	for (int i = 0; i < Pattern.PointCloud.Num(); i++)
	{
		Params.InputPattern[i] = Pattern.PointCloud[i];
	}

	Params.PatternSize = Pattern.Size;
	Params.PatternPointNum = Pattern.PointCloud.Num();
	Params.InputTexture = DensityTexture;
	Params.DepthTexture = DepthTexture;
	Params.PlacementSDF = PlacementSDFTexture;
	Params.InputSDF = InputSDFTexture;
	Params.OutputSDF = SDFSeedUAV;
	Params.LinearSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
	Params.CenterOffset = StartCell;
	Params.BasicHeight = BasicHeight;
	Params.TotalRect = TotalRect;
	Params.ClipRect = DirtyRect;
	Params.Ratio = Ratio;
	Params.RadiusScale = RadiusScale;
	Params.FlipY = FlipY ? 1 : 0;	
	Params.OutputPointCloud = OutputBufferUAV;

	UE_LOG(LogTemp, Warning, TEXT("StartCell :%s \n TotalRect: %s \n DirtyCellNumber: %s\n"), *StartCell.ToString(), *TotalRect.ToString(), *DirtyCellNumber.ToString());

	FComputeShaderUtils::Dispatch(RHICmdList, GPUScatteringCS, Params, FIntVector(DirtyCellNumber.X, DirtyCellNumber.Y, 1));

	
}
static void JumpFlood_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FRHITexture2D* InputSeedTexture,
	FUnorderedAccessViewRHIRef OutputSDFRTUAV,
	FVector4 TotalRect,
	FVector4 SimulateRect,
	bool SubstractSeedRadius,
	float LengthScale,
	ERHIFeatureLevel::Type FeatureLevel)
{

	FIntPoint GroupSize = FIntPoint(8, 8);

	FVector2D TotalRectSize = FVector2D(TotalRect.Z - TotalRect.X, TotalRect.W - TotalRect.Y);
	
	FVector2D Temp;

	Temp = (FVector2D(SimulateRect.X, SimulateRect.Y) - FVector2D(TotalRect.X, TotalRect.Y)) / TotalRectSize;
	Temp *= InputSeedTexture->GetSizeXY();
	Temp /= GroupSize;
	FIntPoint StartGroup =  FIntPoint(floorf(Temp.X), floorf(Temp.Y));

	Temp = (FVector2D(SimulateRect.Z, SimulateRect.W) - FVector2D(TotalRect.X, TotalRect.Y)) / TotalRectSize;
	Temp *= InputSeedTexture->GetSizeXY();
	Temp /= GroupSize;
	FIntPoint EndGroup = FIntPoint(ceilf(Temp.X), ceilf(Temp.Y));

	FIntRect IntSimulateRect = FIntRect(StartGroup * GroupSize, EndGroup * GroupSize-FIntPoint(1,1));

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
	JFAInitParams.SimulateRect = IntSimulateRect;
	JFAInitParams.Inverse = SubstractSeedRadius ? 0 : 1;


	FIntVector GroupCount;
	GroupCount.X = EndGroup.X - StartGroup.X;
	GroupCount.Y = EndGroup.Y - StartGroup.Y;
	GroupCount.Z = 1;

	FComputeShaderUtils::Dispatch(RHICmdList, JFAInitCS, JFAInitParams, GroupCount);
	//Debug


	//JFAStep
	uint32 Step = FMath::Max(GroupCount.X*GroupSize.X, GroupCount.Y * GroupSize.Y) / 2;
	bool PingPong = false;
	JFAStepParams.InputSeed = InputSeedTexture;
	JFAStepParams.SimulateRect = IntSimulateRect;
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
	JFASDFOutputParams.SimulateRect = IntSimulateRect;
	JFASDFOutputParams.SubstractSeedRadius = SubstractSeedRadius?1:0;
	JFASDFOutputParams.LengthScale = LengthScale;

	FComputeShaderUtils::Dispatch(RHICmdList, JFASDFOutputCS, JFASDFOutputParams, GroupCount);

	
}

void ScatterFoliagePass_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	ERHIFeatureLevel::Type FeatureLevel,
	FBiomePipelineContext& Context		
)
{
	check(IsInRenderingThread());

	
	FRHITexture2D* SDFTexture = Context.OutputDistanceFieldResource->GetRenderTargetTexture();
	

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

	float LengthScale = Context.TotalRect.Z - Context.TotalRect.X;
	
	double start, end;
	start = FPlatformTime::Seconds();

	float MaxSpeciesRadiusScale = 0;
	for (int i = 0; i < Context.SpeciesProxys.Num(); i++)
	{
		MaxSpeciesRadiusScale = FMath::Max(Context.SpeciesProxys[i].Radius, MaxSpeciesRadiusScale);
	}
	
	FVector4 SimulateRect = Context.DirtyRect + FVector4(-MaxSpeciesRadiusScale, MaxSpeciesRadiusScale, -MaxSpeciesRadiusScale, MaxSpeciesRadiusScale) * 100;
	
	SimulateRect.X = FMath::Max(SimulateRect.X, Context.TotalRect.X);
	SimulateRect.Y = FMath::Max(SimulateRect.Y, Context.TotalRect.Y);
	SimulateRect.Z = FMath::Min(SimulateRect.Z, Context.TotalRect.Z);
	SimulateRect.W = FMath::Min(SimulateRect.W, Context.TotalRect.W);

	JumpFlood_RenderThread
	(
		RHICmdList,
		Context.PlacementMapResource->GetRenderTargetTexture(),
		PlacementSDFRTUAV,
		Context.TotalRect,
		SimulateRect,
		false,
		LengthScale,
		FeatureLevel 
	);
	//uint32 Stride;
	//RHILockTexture2D(PlacementSDFRT, 0, EResourceLockMode::RLM_ReadOnly, Stride, false);
	//FRHICopyTextureInfo CopyInfo;
	//CopyInfo.Size.X = TexSize.X;
	//CopyInfo.Size.Y = TexSize.Y;
	//CopyInfo.Size.Z = 1;
	//RHICmdList.CopyTexture(PlacementSDFRT, SDFTexture, CopyInfo);
	//RHIUnlockTexture2D(PlacementSDFRT, 0, false);
	end = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Warning, TEXT("Placement JumpFlood excute in %f seconds."), end - start);
	for (int i = 0; i < Context.SpeciesProxys.Num(); i++)
	{
	
		//Generate Point Cloud Data
		float Ratio= Context.SpeciesProxys[i].Ratio;
		float RadiusScale= Context.SpeciesProxys[i].Radius;
		
		start = FPlatformTime::Seconds();

		Context.ReadBackBuffers.AddDefaulted();
		
		RealTImeScatterGPU_RenderThread
		(
			RHICmdList,
			Context.DensityResources[i]->GetRenderTargetTexture(),
			Context.DepthMapResource->GetRenderTargetTexture(),
			PlacementSDFRT,
			OutputSDFRT,
			SDFSeedTextureUAV,
			Context.Pattern,
			Context.BasicHeight,
			Context.TotalRect,
			SimulateRect,
			Context.DirtyRect,
			Ratio,
			RadiusScale,
			Context.FlipY,
			FeatureLevel,
			Context.ReadBackBuffers.Last()
		);

		end = FPlatformTime::Seconds();
		UE_LOG(LogTemp, Warning, TEXT("Point Cloud Generate excute in %f seconds."), end - start);

		start = FPlatformTime::Seconds();
		//Generate Collision SDF
		JumpFlood_RenderThread
		(
			RHICmdList,
			SDFSeedTexture,
			OutputSDFRTUAV,
			Context.TotalRect,
			SimulateRect,
			true,
			LengthScale,
			FeatureLevel
		);

		end = FPlatformTime::Seconds();
		UE_LOG(LogTemp, Warning, TEXT("Collision SDF JumpFlood excute in %f seconds."), end - start);
	}

	/*for (int i = 0; i < ScatterPointCloud.Num(); i++)
	{
		ReadBackBuffers[i].ReadBackToArray(ScatterPointCloud[i].ScatterPoints);
	}*/
	
}

void BiomeGeneratePipeline_RenderThread(FRHICommandListImmediate& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, TArray<FBiomePipelineContext>& BiomePipelineContext)
{
	double start = FPlatformTime::Seconds();
	
	
	for (FBiomePipelineContext& Context : BiomePipelineContext)
	{
		//Context.PlacementMapResource->FlushDeferredResourceUpdate(RHICmdList);
		//for(FTextureRenderTargetResource* DensityResource : Context.DensityResources)
		//	DensityResource->FlushDeferredResourceUpdate(RHICmdList);

		ScatterFoliagePass_RenderThread
		(
			RHICmdList,
			FeatureLevel,
			Context
		);
	}

	double end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("Dispatch executed in %f seconds."), end - start);
	
	start = FPlatformTime::Seconds();
	for (FBiomePipelineContext& Context : BiomePipelineContext)
	{
		for (int i = 0; i < Context.ScatterPointCloud.Num();i++)
		{
			Context.ReadBackBuffers[i].ReadBackToArray(Context.ScatterPointCloud[i].ScatterPoints);
		}
	}

	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("Total ReadBack executed in %f seconds."), end - start);
}
void UReaTimeScatterLibrary::BiomeGeneratePipeline(
	UObject* WorldContextObject,
	TArray<FBiomePipelineContext>& BiomePipelineContext
)
{
	check(IsInGameThread());

	//PlacementMap->Resource->GetTexture2DResource
	ERHIFeatureLevel::Type FeatureLevel = WorldContextObject->GetWorld()->Scene->GetFeatureLevel();
	

	ENQUEUE_RENDER_COMMAND(CaptureCommand)
		(
			[FeatureLevel,&BiomePipelineContext](FRHICommandListImmediate& RHICmdList)
			{
				

				BiomeGeneratePipeline_RenderThread
				(
					RHICmdList,
					FeatureLevel,
					BiomePipelineContext
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
