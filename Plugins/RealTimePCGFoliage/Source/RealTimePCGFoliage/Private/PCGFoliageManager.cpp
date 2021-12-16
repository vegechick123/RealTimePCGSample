// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGFoliageManager.h"
#include "Components/BrushComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "RealTimePCGFoliageEdMode.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "RealTimePCGUtils.h"
// Sets default values
APCGFoliageManager::APCGFoliageManager()
{
	//ConstructorHelpers::FClassFinder<UTextureCollectComponentBase> BPClass(TEXT("Blueprint'/RealTimePCGFoliage/BluePrint/Demo.Demo_C'"));
	UClass* BPClass =LoadClass<UTextureCollectComponentBase>(GetWorld(),TEXT("Blueprint'/RealTimePCGFoliage/BluePrint/TextureCollectComponent.TextureCollectComponent_C'"));
	TextureCollectComponent = (UTextureCollectComponentBase*)CreateDefaultSubobject(TEXT("TextureCollectComp"), UTextureCollectComponentBase::StaticClass(), BPClass, false, false);
	PlacementCopyMaterial = LoadObject<UMaterial>(this, TEXT("'/RealTimePCGFoliage/Material/M_PlacementCopy.M_PlacementCopy'"), nullptr, LOAD_None, nullptr);
	//LoadObject<UMaterial>();
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
	FillBiomeData();
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
	FVector4 DirtyRect;
	if (!bPartialUpdate)
		DirtyRect = GetTotalRect();
	double start = FPlatformTime::Seconds();

	TArray<FDesiredFoliageInstance> OutFoliageInstances;
	for (int i = 0; i < Biomes.Num(); i++)
	{
		float ExtraRadius = 0;
		for (USpecies* CurrentSpecies : Biomes[i]->Species)
		{
			ExtraRadius += CurrentSpecies->Radius;
		}
		//ExtraRadius *= 5;
		DirtyRect = FVector4(DirtyCenter-(ExtraRadius+DirtyRadius), DirtyCenter+ (ExtraRadius + DirtyRadius));
		BiomeGeneratePipeline(Biomes[i], BiomeData[i], OutFoliageInstances, DirtyRect);
		CleanPreviousFoliage(OutFoliageInstances, DirtyRect);
	}
	//bool Result = ExecuteSimulation(OutFoliageInstances, DirtyRect);
	
	double end1 = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Warning, TEXT("ExecuteSimulation executed in %f seconds."), end1 - start);

	CleanPreviousFoliage(OutFoliageInstances,DirtyRect);
	double end2 = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Warning, TEXT("CleanPreviousFoliage executed in %f seconds."), end2 - end1);

	FRealTimePCGFoliageEdMode::AddInstances(GetWorld(),OutFoliageInstances);
	double end3 = FPlatformTime::Seconds();
	
	UE_LOG(LogTemp, Warning, TEXT("AddInstances in %f seconds."), end3 - end2);
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
			DesiredInst->Rotation = FQuat();
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
	TSet<const UFoliageType*> Set;
	for (FDesiredFoliageInstance Instance: OutFoliageInstances)
	{		
		Set.Add(Instance.FoliageType);
	}
	for (const UFoliageType* FoliageType : Set)
	{
		FRealTimePCGFoliageEdMode::CleanProcedualFoliageInstance(GetWorld(),ProceduralGuid,FoliageType, DirtyRect);
	}
}
void APCGFoliageManager::FillBiomeData()
{
	BiomeData.Empty();
	for (UBiome* Biome : Biomes)
	{
		BiomeData.Add(FBiomeData(this, Biome, TexSize));
	}
	Modify();
}

void APCGFoliageManager::BiomeGeneratePipeline(UBiome* InBiome, FBiomeData& InBiomeData, TArray<FDesiredFoliageInstance>& OutFoliageInstances,FVector4 DirtyRect)
{
	InBiomeData.FillDensityMaps();
	for (int i = 0; i < InBiome->Species.Num(); i++)
	{
		UMaterialInstanceDynamic* DensityCaculateMID = UMaterialInstanceDynamic::Create(InBiome->Species[i]->DensityCalculateMaterial,GetTransientPackage());
		
		TextureCollectComponent->SetUpMID(DensityCaculateMID);		
		DensityCaculateMID->SetTextureParameterValue("Placement", InBiomeData.PlacementMap); 		
		DensityCaculateMID->SetTextureParameterValue("Clean", InBiomeData.CleanMaps[i]);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, InBiomeData.DensityMaps[i], DensityCaculateMID);
	}
	UMaterialInstanceDynamic* PlacementCopyMID = UMaterialInstanceDynamic::Create(PlacementCopyMaterial,GetTransientPackage());
	UTextureRenderTarget2D* InitPlacementMap  =RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(nullptr,"InitPlacementMap",TexSize,ETextureRenderTargetFormat::RTF_R32f,FLinearColor::Black);
	PlacementCopyMID->SetTextureParameterValue("Texture", InBiomeData.PlacementMap);
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, InitPlacementMap, PlacementCopyMID);

	FlushRenderingCommands();
	TArray<FSpeciesProxy>  SpeciesProxys = CreateSpeciesProxy(InBiome);

	FScatterPattern Pattern = UReaTimeScatterLibrary::GetDefaultPattern();
	TArray<FSpeciesProxy> CollisionPasses = CreateSpeciesProxy(InBiome);
	
	TArray<FScatterPointCloud> ScatterPointCloud;
	ScatterPointCloud.AddDefaulted(CollisionPasses.Num());
	TArray<FDesiredFoliageInstance> OutInstances;
		
	
	UReaTimeScatterLibrary::RealTImeScatterGPU(this, InitPlacementMap,InBiomeData.DensityMaps, DistanceField,GetTotalRect(), DirtyRect, Pattern, CollisionPasses, ScatterPointCloud, false);
	
	FTransform WorldTM;
	ConvertToFoliageInstance(InBiome,ScatterPointCloud, WorldTM, 2000, OutFoliageInstances);
}

FVector4 APCGFoliageManager::GetTotalRect()
{
	FVector Scale = Landscape->GetTransform().GetScale3D() * 505;
	FVector2D Size = FVector2D(Scale.X, Scale.Y);
	FVector4 TotalRect = FVector4(-Size.X, -Size.Y, Size.X, Size.Y) / 2;
	return TotalRect;
}



