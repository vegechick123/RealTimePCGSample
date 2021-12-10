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
		GenerateProceduralContent();
		ReBuild = false;
	}
}

bool APCGFoliageManager::GenerateProceduralContent()
{
#if WITH_EDITOR
	double start = FPlatformTime::Seconds();

	Density = RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(Density,TEXT("Density"), TexSize, ETextureRenderTargetFormat::RTF_R16f,FLinearColor::Black);

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
bool APCGFoliageManager::ExecuteSimulation(TArray<FDesiredFoliageInstance>& OutFoliageInstances)
{
	FScatterPattern Pattern = UReaTimeScatterLibrary::GetDefaultPattern();
	TArray<FSpeciesProxy> CollisionPasses; //= CreateSpeciesProxy();
	FVector Scale = Landscape->GetTransform().GetScale3D()*500;
	FVector2D Size = FVector2D(Scale.X, Scale.Y);
	TArray<FScatterPointCloud> ScatterPointCloud;
	TArray<FDesiredFoliageInstance> OutInstances;

	BiomeGeneratePipeline(Biomes[0], BiomeData[0], OutFoliageInstances);
	double start = FPlatformTime::Seconds();

	
	//UReaTimeScatterLibrary::ScatterWithCollision(this, Density,DistanceField,- Size/2, Size / 2,Pattern,CollisionPasses, ScatterPointCloud,false,true);
	double end1 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("ScatterWithCollision executed in %f seconds."), end1 - start);
	
	FTransform WorldTM;
	//ConvertToFoliageInstance(ScatterPointCloud,WorldTM,2000,OutFoliageInstances);
	double end2 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("ConvertToFoliageInstance executed in %f seconds."), end2 - end1);
	
	return true;
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
void APCGFoliageManager::FillBiomeData()
{
	BiomeData.Empty();
	for (UBiome* Biome : Biomes)
	{
		BiomeData.Add(FBiomeData(this, Biome, TexSize));
	}
	Modify();
}

void APCGFoliageManager::BiomeGeneratePipeline(UBiome* InBiome, FBiomeData& InBiomeData, TArray<FDesiredFoliageInstance>& OutFoliageInstances)
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
	FVector Scale = Landscape->GetTransform().GetScale3D() * 500;
	FVector2D Size = FVector2D(Scale.X, Scale.Y);
	TArray<FScatterPointCloud> ScatterPointCloud;
	ScatterPointCloud.AddDefaulted(CollisionPasses.Num());
	TArray<FDesiredFoliageInstance> OutInstances;
		
	
	UReaTimeScatterLibrary::RealTImeScatterGPU(this, InitPlacementMap,InBiomeData.DensityMaps, DistanceField, -Size / 2, Size / 2, Pattern, CollisionPasses, ScatterPointCloud, false);
	
	FTransform WorldTM;
	ConvertToFoliageInstance(InBiome,ScatterPointCloud, WorldTM, 2000, OutFoliageInstances);
}



