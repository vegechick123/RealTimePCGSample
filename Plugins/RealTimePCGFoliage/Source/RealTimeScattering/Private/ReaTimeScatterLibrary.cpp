// Fill out your copyright notice in the Description page of Project Settings.


#include "ReaTimeScatterLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GPUScatteringCS.h"
#include "RenderGraphUtils.h"
#include "Async/ParallelFor.h"
void FScatterPointCloud::BuildKdTree()
{
	for (FScatterPoint P : ScatterPoints)
	{
		Points.Add(P.Location());
	}
	KdTree = FKdTree::BuildKdTree(Points);
}
void FScatterPointCloud::CleanKdTree()
{
	delete KdTree;
	KdTree = nullptr;
}
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
static void RealTImeScatterGPU_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FRHITexture* DensityTexture,
	const FScatterPattern& Pattern,
	FVector4 Rect,
	float Ratio,
	float RadiusScale,
	bool FlipY,
	ERHIFeatureLevel::Type FeatureLevel,
	TArray<FVector2D>& OutputPointCloud
)
{

	check(IsInRenderingThread());
	FVector2D Size = FVector2D(Rect.Z, Rect.W) - FVector2D(Rect.X, Rect.Y);
	FVector2D Temp = Size / (Pattern.Size * RadiusScale);
	int PerPointsCnt = Pattern.PointCloud.Num();
	FIntPoint CellNumber = FIntPoint(ceilf(Temp.X), ceilf(Temp.Y));
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
	Params.LinearSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
	Params.Rect = Rect;
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

	FComputeShaderUtils::Dispatch(RHICmdList, GPUScatteringCS, Params, FIntVector(CellNumber.X, CellNumber.Y, 1));

	uint32 Count = *(uint32*)RHILockStructuredBuffer(InstanceCountBuffer, 0, sizeof(uint32), EResourceLockMode::RLM_ReadOnly);
	RHIUnlockStructuredBuffer(InstanceCountBuffer.GetReference());
	//OutputPointCloud.Points
	if (Count != 0)
	{
		//TODO
		TArray<FVector2D>& OutputData = OutputPointCloud;
		OutputData.Reset(Count);
		OutputData.AddUninitialized(Count);
		FScatterPoint* srcptr = (FScatterPoint*)RHILockStructuredBuffer(OutputBuffer, 0, sizeof(FScatterPoint) * Maxnum, EResourceLockMode::RLM_ReadOnly);
		//
		FMemory::Memcpy(OutputData.GetData(), srcptr, sizeof(FScatterPoint) * Count);
		RHIUnlockStructuredBuffer(OutputBuffer.GetReference());
		//
	}
}
void UReaTimeScatterLibrary::RealTImeScatterGPU(UObject* WorldContextObject,UTextureRenderTarget2D* Mask, FVector2D BottomLeft, FVector2D TopRight, const FScatterPattern& Pattern, TArray<FVector2D>& Result, float RadiusScale, float Ratio, bool FlipY)
{
	check(IsInGameThread());

	//UWorld* world = GetWorld();

	if (!Mask||!WorldContextObject)
	{
		/*FMessageLog("Blueprint").Warning(
				LOCTEXT("UGraphicToolsBlueprintLibrary::DrawCheckerBoard","DrawUVDisplacementToRenderTarget: Output render target is required."));*/
		return;
	}
	
	FRHITexture* DensityResource = Mask->TextureReference.TextureReferenceRHI->GetTextureReference()->GetReferencedTexture();
	ERHIFeatureLevel::Type FeatureLevel = WorldContextObject->GetWorld()->Scene->GetFeatureLevel();
	ENQUEUE_RENDER_COMMAND(CaptureCommand)
		(
			[DensityResource, FeatureLevel,&Pattern,BottomLeft,TopRight,Ratio,RadiusScale,FlipY,&Result](FRHICommandListImmediate& RHICmdList)
			{
				RealTImeScatterGPU_RenderThread
				(
					RHICmdList,
					DensityResource,
					Pattern,
					FVector4(BottomLeft.X, BottomLeft.Y,TopRight.X, TopRight.Y),
					Ratio,
					RadiusScale,
					FlipY,
					FeatureLevel,
					Result
				);
			}
	);
}
void UReaTimeScatterLibrary::CollisionDiscard(const TArray<FVector2D>& InData, float InRadius, TArray<FVector2D>& OutData, const TArray<FScatterPointCloud>& Tree,int Count)
{
	FCriticalSection Mutex;
	ParallelFor(InData.Num(), [&](int index)
		{
			const FVector2D& P = InData[index];
			bool flag = false;
			for (int i = 0; i < Count; i++)
			{
				const FScatterPointCloud& CollisionPoints = Tree[i];
				if (CollisionPoints.KdTree->FindNearestPoint(P) < (InRadius + CollisionPoints.Radius)*100/ 2)
				{
					flag = true;
					break;
				}
			}
			if (!flag)
			{
				Mutex.Lock();
				OutData.Add(P);
				Mutex.Unlock();
			}

		});
}
void UReaTimeScatterLibrary::ScatterWithCollision(UObject* WorldContextObject,UTextureRenderTarget2D* Mask, FVector2D BottomLeft, FVector2D TopRight, const FScatterPattern& Pattern, const TArray<FCollisionPass>& InData, TArray<FScatterPointCloud>& OutData, bool FlipY,bool UseGPU)
{
	if (!Mask||InData.Num() == 0)
		return;
	TArray<FColor> Samples;
	FIntRect SampleRect(0, 0, Mask->SizeX, Mask->SizeY);
	if (!UseGPU)
	{
		FReadSurfaceDataFlags ReadSurfaceDataFlags;		
		Samples.SetNumUninitialized(Mask->SizeX * Mask->SizeY);
		FRenderTarget* RenderTarget = Mask->GameThread_GetRenderTargetResource();
		if (!RenderTarget->ReadPixelsPtr(Samples.GetData(), ReadSurfaceDataFlags, SampleRect))
		{
			return;
		}
	}
	
	TArray<FRenderCommandFence> Fences;
	TArray<TArray<FVector2D>> TempData;
	OutData.AddDefaulted(InData.Num());
	
	Fences.AddDefaulted(InData.Num());
	TempData.AddDefaulted(InData.Num() - 1);

	for (int i = 0; i < InData.Num(); i++)
	{
		
		OutData[i].Radius = InData[i].Radius;
		if (i == 0)
		{
			if (UseGPU)
			{
				RealTImeScatterGPU(WorldContextObject, Mask, BottomLeft, TopRight, Pattern, OutData[i].Points, InData[i].Radius, InData[i].Ratio, FlipY);
				Fences[i].BeginFence();
			}
			else
			{
				RealTImeScatter(Samples, SampleRect.Size(), BottomLeft, TopRight, Pattern, OutData[i].Points, InData[i].Radius, InData[i].Ratio, FlipY);
			}
			
		}
		else
		{;
			if (UseGPU)
			{
				RealTImeScatterGPU(WorldContextObject, Mask, BottomLeft, TopRight, Pattern, TempData[i-1], InData[i].Radius, InData[i].Ratio, FlipY);
				Fences[i].BeginFence();
			}
			else
			{
				RealTImeScatter(Samples, SampleRect.Size(), BottomLeft, TopRight, Pattern, TempData[i-1], InData[i].Radius, InData[i].Ratio, FlipY);
			}
				
			
		}		
	}

	Fences[0].Wait();
	for (int i = 1; i < InData.Num(); i++)
	{
		Fences[i].Wait();
		OutData[i-1].BuildKdTree();
		CollisionDiscard(TempData[i-1], InData[i].Radius, OutData[i].Points, OutData,i);
	}

	for (int i = 0; i < InData.Num() - 1; i++)
	{
		OutData[i].CleanKdTree();
	}
}
void UReaTimeScatterLibrary::ConvertToTransform(FVector2D InBottomLeft, FVector2D InTopRight, FVector2D OutBottomLeft, FVector2D OutTopRight, const TArray<FScatterPointCloud>& InData, TArray<FTransform>& OutData)
{
	int TransformNum = 0;
	for (const FScatterPointCloud& tree : InData)
	{
		TransformNum += tree.Points.Num();
	}
	int Index = 0;
	OutData.AddDefaulted(TransformNum);
	for (const FScatterPointCloud& tree : InData)
	{
		for (FVector2D Pos2D : tree.Points)
		{
			FVector Position;
			Pos2D = Pos2D;
			Position.X = Pos2D.X;
			Position.Y = Pos2D.Y;
			Position.Z = 0;
			OutData[Index].SetLocation(Position);
			OutData[Index++].SetScale3D(FVector(tree.Radius));
		}
	}
}
