// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGFoliageManager.h"
#include "Components/BrushComponent.h"

#include "RealTimePCGFoliageEdMode.h"
// Sets default values
APCGFoliageManager::APCGFoliageManager()
{
	UClass* BPClass =LoadClass<UTextureCollectComponentBase>(nullptr,TEXT("Blueprint'/RealTimePCGFoliage/BluePrint/TextureCollectComponent.TextureCollectComponent_C'"));
	TextureCollectComponent = (UTextureCollectComponentBase*)CreateDefaultSubobject(TEXT("TextureCollectComp"), UTextureCollectComponentBase::StaticClass(), BPClass, false, false);
}

// Called when the game starts or when spawned
void APCGFoliageManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APCGFoliageManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APCGFoliageManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (Landscape != nullptr)
	{
		SetActorScale3D(Landscape->GetActorScale3D());
	}
	if (ReBuild)
	{
		GenerateProceduralContent();
		ReBuild = false;
	}
}

bool APCGFoliageManager::GenerateProceduralContent()
{
#if WITH_EDITOR
	double start = FPlatformTime::Seconds();

	Density = GetOrCreateTransientRenderTarget2D(Density,TEXT("Density"), TexSize, ETextureRenderTargetFormat::RTF_R32f,FLinearColor::Black);

	TextureCollectComponent->RenderDensityTexture(Mask,Density,DensityCalculateMaterial);

	TArray<FDesiredFoliageInstance> OutFoliageInstances;

	bool Result = ExecuteSimulation(OutFoliageInstances);
	
	double end1 = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Warning, TEXT("ExecuteSimulation executed in %f seconds."), end1 - start);
	// put code you want to time here.

	
	//
	CleanPreviousFoliage(OutFoliageInstances);
	double end2 = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Warning, TEXT("CleanPreviousFoliage executed in %f seconds."), end2 - end1);

	FRealTimePCGFoliageEdMode::AddInstances(GetWorld(),OutFoliageInstances);
	double end3 = FPlatformTime::Seconds();
	
	UE_LOG(LogTemp, Warning, TEXT("AddInstances in %f seconds."), end3 - end2);
	return Result;
#endif
	return false;
}

TArray<FCollisionPass> APCGFoliageManager::GetCollisionPasses()
{
	TArray<FCollisionPass> Result;
	for (int i = 0; i < PCGFoliageTypes.Num(); i++)
	{
		Result.Add(PCGFoliageTypes[i].CollisionPass);
	}
	return Result;

}
bool APCGFoliageManager::ExecuteSimulation(TArray<FDesiredFoliageInstance>& OutFoliageInstances)
{
	FScatterPattern Pattern = UReaTimeScatterLibrary::GetDefaultPattern();
	TArray<FCollisionPass> CollisionPasses = GetCollisionPasses();
	FVector Scale = Landscape->GetTransform().GetScale3D()*500;
	FVector2D Size = FVector2D(Scale.X, Scale.Y);
	TArray<FScatterPointCloud> ScatterPointCloud;
	TArray<FDesiredFoliageInstance> OutInstances;

	double start = FPlatformTime::Seconds();

	
	UReaTimeScatterLibrary::ScatterWithCollision(this, Density, DistanceSeed,DistanceField,- Size/2, Size / 2,Pattern,CollisionPasses, ScatterPointCloud,false,true);
	double end1 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("ScatterWithCollision executed in %f seconds."), end1 - start);
	
	FTransform WorldTM;
	ConvertToFoliageInstance(ScatterPointCloud,WorldTM,2000,OutFoliageInstances);
	double end2 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("ConvertToFoliageInstance executed in %f seconds."), end2 - end1);
	
	return true;
}

void APCGFoliageManager::ConvertToFoliageInstance(const TArray<FScatterPointCloud>& ScatterPointCloud, const FTransform& WorldTM, const float HalfHeight, TArray<FDesiredFoliageInstance>& OutInstances) const
{

	for (int i =0;i < ScatterPointCloud.Num();i++)
	{
		const FScatterPointCloud& Points = ScatterPointCloud[i];

		for (const FScatterPoint& Instance : Points.ScatterPoints)
		{
			FVector StartRay = FVector(Instance.LocationX, Instance.LocationY, 0) + WorldTM.GetLocation();
			StartRay.Z += HalfHeight;
			FVector EndRay = StartRay;
			EndRay.Z -= (HalfHeight * 2.f + 10.f);	//add 10cm to bottom position of raycast. This is needed because volume is usually placed directly on geometry and then you get precision issues

			FDesiredFoliageInstance* DesiredInst = new (OutInstances)FDesiredFoliageInstance(StartRay, EndRay, Points.Radius);
			DesiredInst->Rotation = FQuat();
			DesiredInst->ProceduralGuid = ProceduralGuid;
			DesiredInst->FoliageType = PCGFoliageTypes[i].FoliageType;
			DesiredInst->Age = 0;
			DesiredInst->TraceRadius = 100;
			DesiredInst->ProceduralVolumeBodyInstance = nullptr;
			DesiredInst->PlacementMode = EFoliagePlacementMode::Procedural;
		}
	}
}

void APCGFoliageManager::RemoveProceduralContent(bool InRebuildTree)
{
}

void APCGFoliageManager::CleanPreviousFoliage(const TArray<FDesiredFoliageInstance>& OutFoliageInstances)
{
	TSet<const UFoliageType*> Set;
	for (FDesiredFoliageInstance Instance: OutFoliageInstances)
	{		
		Set.Add(Instance.FoliageType);
	}
	for (const UFoliageType* FoliageType : Set)
	{
		FRealTimePCGFoliageEdMode::CleanProcedualFoliageInstance(GetWorld(),ProceduralGuid,FoliageType);
	}
}

UTextureRenderTarget2D* APCGFoliageManager::GetOrCreateTransientRenderTarget2D(UTextureRenderTarget2D* InRenderTarget, FName InRenderTargetName, const FIntPoint& InSize, ETextureRenderTargetFormat InFormat,
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