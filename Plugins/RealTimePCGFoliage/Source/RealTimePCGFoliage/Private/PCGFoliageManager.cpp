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



APCGFoliageManager::APCGFoliageManager()
{
	UClass* BPClass =LoadClass<UTextureCollectComponentBase>(GetWorld(),TEXT("Blueprint'/RealTimePCGFoliage/BluePrint/TextureCollectComponent.TextureCollectComponent_C'"));
	TextureCollectComponent = (UTextureCollectComponentBase*)CreateDefaultSubobject(TEXT("TextureCollectComp"), UTextureCollectComponentBase::StaticClass(), BPClass, false, false);
	PlacementCopyMaterial = LoadObject<UMaterial>(this, TEXT("'/RealTimePCGFoliage/Material/M_PlacementCopy.M_PlacementCopy'"), nullptr, LOAD_None, nullptr);
	BiomePreviewMaterial = LoadObject<UMaterial>(this, TEXT("'/RealTimePCGFoliage/Material/M_BiomePreview.M_BiomePreview'"), nullptr, LOAD_None, nullptr);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	SceneCaptureHeight = 5000.0f;

	SceneCaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent2D"));
	SceneCaptureComponent2D->CreationMethod = EComponentCreationMethod::Native;

	

	SceneCaptureComponent2D->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	SceneCaptureComponent2D->SetRelativeRotation(FRotator(-90.0f, 0.0f, -90.0f));
	SceneCaptureComponent2D->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	SceneCaptureComponent2D->SetRelativeLocation(FVector(0, 0, SceneCaptureHeight));

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
	if (ReBuild)
	{
		GenerateProceduralContent(false);
		ReBuild = false;
	}
}

bool APCGFoliageManager::GenerateProceduralContent(bool bPartialUpdate, FVector2D EditedCenter, float EditedRadius)
{
#if WITH_EDITOR
	double start,end;
	start = FPlatformTime::Seconds();
	if(AutoCaptureLandscape)
		CaptureLandscape();
	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("CaptureLandscape executed in %f seconds."), end - start);
	FVector4 EditedRect;
	if (!bPartialUpdate)
		EditedRect = GetLandscapeBound();

	start = FPlatformTime::Seconds();
	TArray<FPotentialInstance> OutFoliageInstances;
	
	ExcuteBiomeGeneratePipeline(OutFoliageInstances, EditedCenter, EditedRadius);

	float MaxExtraRadius=0;

	for (int i = 0; i < Biomes.Num(); i++)
	{
		float ExtraRadius = 0;
		for (USpecies* CurrentSpecies : Biomes[i]->Species)
		{
			ExtraRadius += CurrentSpecies->Radius;
		}	
		MaxExtraRadius = FMath::Max(ExtraRadius,MaxExtraRadius);
	}
	TArray<UFoliageType*> FoliageTypes;
	for (UBiome* CurrentBiome : Biomes)
	{
		for (USpecies* CurrentSpecies :CurrentBiome->Species)
		{
			FoliageTypes.Add(CurrentSpecies->FoliageTypes[0]);
		}
	}
	CleanPreviousFoliage(FoliageTypes, FVector4(EditedCenter - (MaxExtraRadius + EditedRadius), EditedCenter + (MaxExtraRadius + EditedRadius)));
	
	end = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Warning, TEXT("SingleBiomeGeneratePipeline executed in %f seconds."), end - start);


	start = FPlatformTime::Seconds();
	FRealTimePCGFoliageEdMode::AddPotentialInstances(GetWorld(),OutFoliageInstances);
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


void APCGFoliageManager::ConvertToFoliageInstance(UBiome* InBiome,const TArray<FScatterPointCloud>& ScatterPointCloud, const FTransform& WorldTM, const float HalfHeight, TArray<FPotentialInstance>& OutInstances) const
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

			FDesiredFoliageInstance DesiredInst(StartRay, EndRay, CurrentSpecies->Radius);
			DesiredInst.Rotation = FQuat::Identity;
			DesiredInst.ProceduralGuid = ProceduralGuid;
			DesiredInst.FoliageType = CurrentSpecies->FoliageTypes[0];
			DesiredInst.Age = 0;
			DesiredInst.TraceRadius = 100;
			DesiredInst.ProceduralVolumeBodyInstance = nullptr;
			DesiredInst.PlacementMode = EFoliagePlacementMode::Procedural;

			FPotentialInstance* PotentialInst = new (OutInstances)FPotentialInstance(Instance.GetLocation(),FVector(0,0,1),nullptr,1.0f,DesiredInst);
		}
	}
}

void APCGFoliageManager::RemoveProceduralContent(bool InRebuildTree)
{
}

void APCGFoliageManager::CleanPreviousFoliage(const TArray<UFoliageType*>& CleanFoliageTypes,FVector4 DirtyRect)
{
	double start, end;
	start = FPlatformTime::Seconds();

	for (const UFoliageType* FoliageType : CleanFoliageTypes)
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
		if (!Biomes[i])
			continue;
		if (BiomeData.Num() <= i)
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
	
	FVector Scale = Landscape->GetTransform().GetScale3D();
	FVector2D Size = FVector2D(Scale.X, Scale.Y) * (GetLandscapeSize());


	SceneCaptureComponent2D->OrthoWidth = FMath::Max(Size.X, Size.Y);
	//Disable Landscape LOD
	SceneCaptureComponent2D->LODDistanceFactor = 0;
	UE_LOG(LogTemp, Warning, TEXT("SceneCaptureComponent2D->OrthoWidth %f"), SceneCaptureComponent2D->OrthoWidth);

	SceneCaptureComponent2D->ShowOnlyActors.Empty();
	SceneCaptureComponent2D->ShowOnlyActorComponents(Landscape);
	
	SceneCaptureComponent2D->TextureTarget = LandscapeNormal;
	SceneCaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_Normal;
	SceneCaptureComponent2D->CaptureScene();
	
	SceneCaptureComponent2D->TextureTarget = LandscapeDepth;
	SceneCaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	SceneCaptureComponent2D->CaptureScene();
	//if (!DebugMID)
	//	DebugMID = UMaterialInstanceDynamic::Create(DebugMaterial,this);
	//DebugMID->SetTextureParameterValue("Depth", LandscapeDepth);
	//UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, DistanceField, DebugMID);

	SceneCaptureComponent2D->ShowOnlyActors.Empty();	
	Modify();
}
void APCGFoliageManager::SingleBiomeGeneratePipeline(UBiome* InBiome, FBiomeData& InBiomeData, TArray<FPotentialInstance>& OutFoliageInstances,FVector4 DirtyRect)
{
	double start, end;
	start = FPlatformTime::Seconds();

	InBiomeData.FillDensityMaps();
	for (int i = 0; i < InBiome->Species.Num(); i++)
	{
		UMaterialInstanceDynamic* DensityCaculateMID = UMaterialInstanceDynamic::Create(InBiome->Species[i]->DensityCalculateMaterial,this);
		
		TextureCollectComponent->SetUpMID(DensityCaculateMID);
		
		DensityCaculateMID->SetScalarParameterValue("CaptureScale", 504.0/512);
		DensityCaculateMID->SetTextureParameterValue("LandscapeNormal", LandscapeNormal);
		DensityCaculateMID->SetTextureParameterValue("Placement", InBiomeData.PlacementMap); 		
		DensityCaculateMID->SetTextureParameterValue("Clean", InBiomeData.CleanMaps[i]);
		DebugDensityMaterial = DensityCaculateMID;
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, InBiomeData.DensityMaps[i], DensityCaculateMID);
	}
	UMaterialInstanceDynamic* PlacementCopyMID = UMaterialInstanceDynamic::Create(PlacementCopyMaterial,GetTransientPackage());
	UTextureRenderTarget2D* InitPlacementMap  = RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(nullptr,"InitPlacementMap",RenderTargetSize,ETextureRenderTargetFormat::RTF_R32f,FLinearColor::Black);
	PlacementCopyMID->SetTextureParameterValue("Texture", InBiomeData.PlacementMap);
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, InitPlacementMap, PlacementCopyMID);

	FlushRenderingCommands();
	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("In SingleBiomeGeneratePipeline DensityMap Calculate in %f seconds."), end - start);

	start = FPlatformTime::Seconds();
	FScatterPattern Pattern = UReaTimeScatterLibrary::GetDefaultPattern();
	TArray<FSpeciesProxy> SpeciesProxys = CreateSpeciesProxy(InBiome);
	
	TArray<FScatterPointCloud> ScatterPointCloud;
	ScatterPointCloud.AddDefaulted(SpeciesProxys.Num());
	TArray<FPotentialInstance> OutInstances;
					
	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("In SingleBiomeGeneratePipeline Scatter Calculate in %f seconds."), end - start);
	FTransform WorldTM;
	
	start = FPlatformTime::Seconds();
	ConvertToFoliageInstance(InBiome,ScatterPointCloud, WorldTM, 2000, OutFoliageInstances);
	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("In SingleBiomeGeneratePipeline Scatter ConvertToFoliageInstance in %f seconds."), end - start);
}
void APCGFoliageManager::ExcuteBiomeGeneratePipeline(TArray<FPotentialInstance>& OutFoliageInstances, FVector2D EditedCenter, float EditedRadius)
{
	double start, end;
	TArray<FBiomePipelineContext> BiomePipelineContext;
	for (int i = 0; i < Biomes.Num(); i++)
	{
		BiomePipelineContext.AddDefaulted();

		BiomeData[i].FillDensityMaps();
		for (int j = 0; j < Biomes[i]->Species.Num(); j++)
		{

			UMaterialInstanceDynamic* DensityCaculateMID = UMaterialInstanceDynamic::Create(Biomes[i]->Species[j]->DensityCalculateMaterial, GetTransientPackage());

			TextureCollectComponent->SetUpMID(DensityCaculateMID);

			DensityCaculateMID->SetScalarParameterValue("CaptureScale", 504.0 / 512);
			DensityCaculateMID->SetTextureParameterValue("LandscapeNormal", LandscapeNormal);
			DensityCaculateMID->SetTextureParameterValue("Placement", BiomeData[i].PlacementMap);
			DensityCaculateMID->SetTextureParameterValue("Clean", BiomeData[i].CleanMaps[j]);
			DebugDensityMaterial = DensityCaculateMID;
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, BiomeData[i].DensityMaps[j], DensityCaculateMID);
		}
		UMaterialInstanceDynamic* PlacementCopyMID = UMaterialInstanceDynamic::Create(PlacementCopyMaterial, GetTransientPackage());
		UTextureRenderTarget2D* InitPlacementMap = RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(nullptr, "InitPlacementMap", RenderTargetSize, ETextureRenderTargetFormat::RTF_R32f, FLinearColor::Black);
		PlacementCopyMID->SetTextureParameterValue("Texture", BiomeData[i].PlacementMap);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, InitPlacementMap, PlacementCopyMID);

		
		float ExtraRadius = 0;
		for (USpecies* CurrentSpecies : Biomes[i]->Species)
		{
			ExtraRadius += CurrentSpecies->Radius;
		}
		FVector4 DirtyRect;
		DirtyRect = FVector4(EditedCenter - (ExtraRadius + EditedRadius), EditedCenter + (ExtraRadius + EditedRadius));

		FBiomePipelineContext& CurrentContext = BiomePipelineContext.Last();
		CurrentContext.Pattern = UReaTimeScatterLibrary::GetDefaultPattern();
		CurrentContext.SpeciesProxys = CreateSpeciesProxy(Biomes[i]);
		CurrentContext.ScatterPointCloud.AddDefaulted(Biomes[i]->Species.Num());
		CurrentContext.TotalRect = GetLandscapeBound();
		CurrentContext.DirtyRect = DirtyRect;
		CurrentContext.BasicHeight = SceneCaptureHeight;
		CurrentContext.DepthMap = LandscapeDepth;
		CurrentContext.PlacementMap = InitPlacementMap;
		CurrentContext.DensityMaps = BiomeData[i].DensityMaps;			
		CurrentContext.OutputDistanceField = DistanceField;
		CurrentContext.FlipY = false;
		CurrentContext.InitRenderThreadResource();
		

	}
	FlushRenderingCommands();
	start = FPlatformTime::Seconds();

	
	UReaTimeScatterLibrary::BiomeGeneratePipeline(this, BiomePipelineContext);

	end = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("In BiomeGeneratePipeline in %f seconds."), end - start);



	FTransform WorldTM;
	for (int i = 0; i < Biomes.Num(); i++)
	{
		ConvertToFoliageInstance(Biomes[i], BiomePipelineContext[i].ScatterPointCloud, WorldTM, 2000, OutFoliageInstances);
	}
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
