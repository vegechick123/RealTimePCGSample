// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGFoliageManager.h"
#include "Components/BrushComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "RealTimePCGFoliageEdMode.h"
#include "Materials/Material.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "RealTimePCGUtils.h"
// Sets default values
APCGFoliageManager::APCGFoliageManager()
{
	UClass* BPClass =LoadClass<UTextureCollectComponentBase>(GetWorld(),TEXT("Blueprint'/RealTimePCGFoliage/BluePrint/TextureCollectComponent.TextureCollectComponent_C'"));
	TextureCollectComponent = (UTextureCollectComponentBase*)CreateDefaultSubobject(TEXT("TextureCollectComp"), UTextureCollectComponentBase::StaticClass(), BPClass, false, false);
	PlacementCopyMaterial = LoadObject<UMaterial>(this, TEXT("'/RealTimePCGFoliage/Material/M_PlacementCopy.M_PlacementCopy'"), nullptr, LOAD_None, nullptr);
	BiomePreviewMaterial = LoadObject<UMaterial>(this, TEXT("'/RealTimePCGFoliage/Material/M_BiomePreview.M_BiomePreview'"), nullptr, LOAD_None, nullptr);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	SceneCaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent2D"));
	SceneCaptureComponent2D->CreationMethod = EComponentCreationMethod::Native;

	

	SceneCaptureComponent2D->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	SceneCaptureComponent2D->SetRelativeRotation(FRotator(-90.0f, 0.0f, -90.0f));
	SceneCaptureComponent2D->SetRelativeScale3D(FVector(0.01f, 0.01f, 0.01f));
	SceneCaptureComponent2D->SetRelativeLocation(FVector(0, 0, 1000));

	SceneCaptureComponent2D->ProjectionType = ECameraProjectionMode::Type::Orthographic;
	SceneCaptureComponent2D->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_Normal;
	SceneCaptureComponent2D->bCaptureEveryFrame = false;
	SceneCaptureComponent2D->bCaptureOnMovement = false;	
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
	RegenerateBiomeData();
	if (Landscape != nullptr)
	{
		SetActorScale3D(Landscape->GetActorScale3D());
	}
	if (ReBuild)
	{
		GenerateProceduralContent(false);
		ReBuild = false;
	}
}

bool APCGFoliageManager::GenerateProceduralContent(bool bPartialUpdate, FVector2D DirtyCenter, float DirtyRadius)
{
#if WITH_EDITOR
	double start,end;
	start = FPlatformTime::Seconds();
	CaptureLandscape();
	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("CaptureLandscape executed in %f seconds."), end - start);
	FVector4 DirtyRect;
	if (!bPartialUpdate)
		DirtyRect = GetLandscapeBound();

	start = FPlatformTime::Seconds();
	TArray<FDesiredFoliageInstance> OutFoliageInstances;
	for (int i = 0; i < Biomes.Num(); i++)
	{
		float ExtraRadius = 0;
		for (USpecies* CurrentSpecies : Biomes[i]->Species)
		{
			ExtraRadius += CurrentSpecies->Radius;
		}

		DirtyRect = FVector4(DirtyCenter-(ExtraRadius+DirtyRadius), DirtyCenter+ (ExtraRadius + DirtyRadius));
		BiomeGeneratePipeline(Biomes[i], BiomeData[i], OutFoliageInstances, DirtyRect);		
		CleanPreviousFoliage(OutFoliageInstances, DirtyRect);
	}
	
	end = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Warning, TEXT("BiomeGeneratePipeline executed in %f seconds."), end - start);


	start = FPlatformTime::Seconds();
	FRealTimePCGFoliageEdMode::AddInstances(GetWorld(),OutFoliageInstances);
	end = FPlatformTime::Seconds();
	
	UE_LOG(LogTemp, Warning, TEXT("AddInstances in %f seconds."), end - start);
	return true;
#endif
	return false;
}

TArray<FSpeciesProxy> APCGFoliageManager::CreateSpeciesProxy(UBiome* InBiome)
{
	TArray<FSpeciesProxy> Result;
	for (USpecies* CurrentSpecies :InBiome->Species)
	{
		FSpeciesProxy Proxy;
		Proxy.Radius = CurrentSpecies->Radius;
		Proxy.Ratio = CurrentSpecies->Ratio;
		Result.Add(Proxy);
	}
	return Result;

}


void APCGFoliageManager::ConvertToFoliageInstance(UBiome* InBiome,const TArray<FScatterPointCloud>& ScatterPointCloud, const FTransform& WorldTM, const float HalfHeight, TArray<FDesiredFoliageInstance>& OutInstances) const
{

	for (int i =0;i < ScatterPointCloud.Num();i++)
	{
		const FScatterPointCloud& Points = ScatterPointCloud[i];
		USpecies* CurrentSpecies = InBiome->Species[i];
		for (const FScatterPoint& Instance : Points.ScatterPoints)
		{
			FVector StartRay = FVector(Instance.LocationX, Instance.LocationY, 0) + WorldTM.GetLocation();
			StartRay.Z += HalfHeight;
			FVector EndRay = StartRay;
			EndRay.Z -= (HalfHeight * 2.f + 10.f);	//add 10cm to bottom position of raycast. This is needed because volume is usually placed directly on geometry and then you get precision issues

			FDesiredFoliageInstance* DesiredInst = new (OutInstances)FDesiredFoliageInstance(StartRay, EndRay, CurrentSpecies->Radius);
			DesiredInst->Rotation = FQuat::Identity;
			DesiredInst->ProceduralGuid = ProceduralGuid;
			DesiredInst->FoliageType = CurrentSpecies->FoliageTypes[0];
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

void APCGFoliageManager::CleanPreviousFoliage(const TArray<FDesiredFoliageInstance>& OutFoliageInstances,FVector4 DirtyRect)
{
	double start, end;
	start = FPlatformTime::Seconds();

	TSet<const UFoliageType*> Set;
	for (FDesiredFoliageInstance Instance: OutFoliageInstances)
	{		
		Set.Add(Instance.FoliageType);
	}
	for (const UFoliageType* FoliageType : Set)
	{
		FRealTimePCGFoliageEdMode::CleanProcedualFoliageInstance(GetWorld(),ProceduralGuid,FoliageType, DirtyRect);
	}
	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("CleanPreviousFoliage executed in %f seconds."), end - start);
}
void APCGFoliageManager::RegenerateBiomeData()
{

	for (int i = 0; i < Biomes.Num(); i++)
	{
		if (BiomeData.Num() < i)
		{
			BiomeData.Add(FBiomeData(this, Biomes[i], TextureSize));
		}
		else
		{
			if (!BiomeData[i].CheckBiome(Biomes[i])|| BiomeData[i].TexSize!=TextureSize)
			{
				BiomeData[i] = FBiomeData(this, Biomes[i], TextureSize);
			}

		}
	}
	Modify();
}
FIntPoint APCGFoliageManager::GetLandscapeSize()
{
	FIntPoint OverallResolution = Landscape->ComputeComponentCounts() * Landscape->ComponentSizeQuads;
	return OverallResolution;
}
void APCGFoliageManager::CaptureLandscape()
{
	LandscapeDepth = RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(LandscapeDepth, "LandscapeDepth", RenderTargetSize, ETextureRenderTargetFormat::RTF_R32f, FLinearColor::Black);
	LandscapeNormal = RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(LandscapeNormal, "LandscapeNormal", RenderTargetSize, ETextureRenderTargetFormat::RTF_RGBA32f, FLinearColor::Black);
	FTransform LandscapeTransform = Landscape->GetTransform();
	FVector Scale = LandscapeTransform.GetScale3D();

	

	const FVector Temp(Scale.X * (float)512, Scale.Y * (float)512, 0.512f);
	SceneCaptureComponent2D->OrthoWidth = FMath::Max(Temp.X, Temp.Y);

	SceneCaptureComponent2D->ShowOnlyActors.Empty();
	SceneCaptureComponent2D->ShowOnlyActorComponents(Landscape);
	

	SceneCaptureComponent2D->TextureTarget = LandscapeDepth;
	SceneCaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	SceneCaptureComponent2D->CaptureScene();
	
	SceneCaptureComponent2D->TextureTarget = LandscapeNormal;
	SceneCaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_Normal;
	SceneCaptureComponent2D->CaptureScene();
	
	SceneCaptureComponent2D->ShowOnlyActors.Empty();	
	Modify();
}
void APCGFoliageManager::BiomeGeneratePipeline(UBiome* InBiome, FBiomeData& InBiomeData, TArray<FDesiredFoliageInstance>& OutFoliageInstances,FVector4 DirtyRect)
{
	double start, end;
	start = FPlatformTime::Seconds();

	InBiomeData.FillDensityMaps();
	for (int i = 0; i < InBiome->Species.Num(); i++)
	{
		UMaterialInstanceDynamic* DensityCaculateMID = UMaterialInstanceDynamic::Create(InBiome->Species[i]->DensityCalculateMaterial,GetTransientPackage());
		
		TextureCollectComponent->SetUpMID(DensityCaculateMID);
		
		DensityCaculateMID->SetScalarParameterValue("CaptureScale", 505.0/512);
		DensityCaculateMID->SetTextureParameterValue("LandscapeNormal", LandscapeNormal);
		DensityCaculateMID->SetTextureParameterValue("Placement", InBiomeData.PlacementMap); 		
		DensityCaculateMID->SetTextureParameterValue("Clean", InBiomeData.CleanMaps[i]);
		DebugDensityMaterial = DensityCaculateMID;
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, InBiomeData.DensityMaps[i], DensityCaculateMID);
	}
	UMaterialInstanceDynamic* PlacementCopyMID = UMaterialInstanceDynamic::Create(PlacementCopyMaterial,GetTransientPackage());
	UTextureRenderTarget2D* InitPlacementMap  =RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(nullptr,"InitPlacementMap",RenderTargetSize,ETextureRenderTargetFormat::RTF_R32f,FLinearColor::Black);
	PlacementCopyMID->SetTextureParameterValue("Texture", InBiomeData.PlacementMap);
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, InitPlacementMap, PlacementCopyMID);

	FlushRenderingCommands();
	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("In BiomeGeneratePipeline DensityMap Calculate in %f seconds."), end - start);

	start = FPlatformTime::Seconds();
	FScatterPattern Pattern = UReaTimeScatterLibrary::GetDefaultPattern();
	TArray<FSpeciesProxy> SpeciesProxys = CreateSpeciesProxy(InBiome);
	
	TArray<FScatterPointCloud> ScatterPointCloud;
	ScatterPointCloud.AddDefaulted(SpeciesProxys.Num());
	TArray<FDesiredFoliageInstance> OutInstances;
			
	UReaTimeScatterLibrary::RealTImeScatterGPU(this, InitPlacementMap,InBiomeData.DensityMaps, DistanceField,GetLandscapeBound(), DirtyRect, Pattern, SpeciesProxys, ScatterPointCloud, false);
	
	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("In BiomeGeneratePipeline Scatter Calculate in %f seconds."), end - start);
	FTransform WorldTM;
	
	start = FPlatformTime::Seconds();
	ConvertToFoliageInstance(InBiome,ScatterPointCloud, WorldTM, 2000, OutFoliageInstances);
	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("In BiomeGeneratePipeline Scatter ConvertToFoliageInstance in %f seconds."), end - start);
}
FVector4 APCGFoliageManager::GetLandscapeBound()
{
	FVector Scale = Landscape->GetTransform().GetScale3D();
	FVector2D Size = FVector2D(Scale.X, Scale.Y)*GetLandscapeSize();
	FVector4 TotalRect = FVector4(-Size.X, -Size.Y, Size.X, Size.Y) / 2;
	return TotalRect;
}

void APCGFoliageManager::DrawPreviewBiomeRenderTarget(UTextureRenderTarget2D* RenderTarget, TArray<FLinearColor> PreviewColors)
{
	UKismetRenderingLibrary::ClearRenderTarget2D(this,RenderTarget,FLinearColor::Black);
	UMaterialInstanceDynamic* PreviewMID = UMaterialInstanceDynamic::Create(BiomePreviewMaterial, this);

	for (int i=0;i< BiomeData.Num();i++)
	{
		const FBiomeData& Data = BiomeData[i];
		PreviewMID->SetTextureParameterValue("Texture", Data.PlacementMap);
		PreviewMID->SetVectorParameterValue("Color", PreviewColors[i]);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, RenderTarget, PreviewMID);
	}
}

TArray<FLinearColor> APCGFoliageManager::GetBiomePreviewColor()
{
	FRandomStream Random(0);
	TArray<FLinearColor> Result;
	for (int i = 0; i < Biomes.Num(); i++)
	{
		const uint8 Hue = (uint8)(Random.FRand() * 255.f);
		Result.Add(FLinearColor::MakeFromHSV8(Hue, 255, 255));
	}
	return Result;
}
