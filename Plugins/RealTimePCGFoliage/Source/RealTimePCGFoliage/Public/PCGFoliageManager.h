// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Landscape.h"
#include "InstancedFoliage.h"
#include "TextureCollectComponentBase.h"
#include "ReaTimeScatterLibrary.h"
#include "GameFramework/Volume.h"
#include "Biome.h"
#include "Species.h"
#include "Engine/TextureRenderTarget2D.h"

#include "PCGFoliageManager.generated.h"

USTRUCT(BlueprintType)
struct REALTIMEPCGFOLIAGE_API FPCGFoliageType
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	UFoliageType* FoliageType;
	UPROPERTY(EditAnywhere)
	FSpeciesProxy CollisionPass;
};
/**
 *
 */

UCLASS()
class REALTIMEPCGFOLIAGE_API APCGFoliageManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APCGFoliageManager();

	UPROPERTY(EditAnywhere)
	bool ReBuild;
	UPROPERTY(EditAnywhere)
	FGuid ProceduralGuid;
	UPROPERTY(EditAnywhere)
	ALandscape* Landscape;
	UPROPERTY(VisibleAnywhere)
	UTextureCollectComponentBase* TextureCollectComponent;
	
	UPROPERTY(EditAnywhere, meta = (MakeStructureDefaultValue = "256,256"))
	FIntPoint TexSize;
	UPROPERTY(EditAnywhere)
	TArray<UBiome*> Biomes;
	UPROPERTY(VisibleAnywhere)
	TArray<FBiomeData> BiomeData;
	
	
	
	UPROPERTY(EditAnywhere, Transient)
	UTextureRenderTarget2D* Mask;
	UPROPERTY(VisibleAnywhere, Transient)
	UTextureRenderTarget2D* Density;
	UPROPERTY(EditAnywhere)
	UMaterialInterface* DensityCalculateMaterial;
	UPROPERTY(EditAnywhere)
	UTextureRenderTarget2D* DistanceSeed;
	UPROPERTY(EditAnywhere)
	UTextureRenderTarget2D* DistanceField;
	UPROPERTY(EditAnywhere)
	UMaterialInterface* PaintMaterial;
	UPROPERTY(VisibleAnywhere)
	UMaterialInterface* DebugPaintMaterial;
	UPROPERTY(EditAnywhere)
	TArray<FPCGFoliageType> PCGFoliageTypes;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	TArray<FSpeciesProxy> CreateSpeciesProxy(UBiome* InBiome);
	bool GenerateProceduralContent();
	bool ExecuteSimulation(TArray<FDesiredFoliageInstance>& OutFoliageInstances);
	void ConvertToFoliageInstance(UBiome* InBiome,const TArray<FScatterPointCloud>& ScatterPointCloud, const FTransform& WorldTM, const float HalfHeight, TArray<FDesiredFoliageInstance>& OutFoliageInstances)const;
	void RemoveProceduralContent(bool InRebuildTree = true);
	void CleanPreviousFoliage(const TArray<FDesiredFoliageInstance>& OutFoliageInstances);
	UFUNCTION(CallInEditor)
	void FillBiomeData();
	void BiomeGeneratePipeline(UBiome* Biome, FBiomeData& InBiomeData, TArray<FDesiredFoliageInstance>& OutFoliageInstances);

};
