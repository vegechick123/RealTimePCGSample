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
class USceneCaptureComponent2D;
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
	bool AutoCaptureLandscape;
	UPROPERTY(EditAnywhere)
	float SceneCaptureHeight;
	UPROPERTY(EditAnywhere)
	FGuid ProceduralGuid;
	UPROPERTY(EditAnywhere)
	ALandscape* Landscape;
	UPROPERTY(VisibleAnywhere)
	UTextureCollectComponentBase* TextureCollectComponent;
	UPROPERTY(VisibleAnywhere)
	USceneCaptureComponent2D* SceneCaptureComponent2D;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MakeStructureDefaultValue = "256,256"))
	FIntPoint TextureSize;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MakeStructureDefaultValue = "2048,2048"))
	FIntPoint RenderTargetSize;
	UPROPERTY(EditAnywhere)
	TArray<UBiome*> Biomes;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	TArray<FBiomeData> BiomeData;
	
	UPROPERTY(VisibleAnywhere)
	UMaterialInterface* PlacementCopyMaterial;
	
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly)
	UTextureRenderTarget2D* LandscapeDepth;
	UPROPERTY(VisibleAnywhere, Transient,BlueprintReadOnly)
	UTextureRenderTarget2D* LandscapeNormal;
	
	UPROPERTY(EditAnywhere)
	UTextureRenderTarget2D* DistanceField;
	UPROPERTY(EditAnywhere)
	UMaterialInterface* DistanceFieldMaterial;
	UPROPERTY(EditAnywhere)
	UTextureRenderTarget2D* DistanceFieldPreview;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* PaintMaterial;
	UPROPERTY(VisibleAnywhere, meta = (Category = "Brush Materials"))
	UMaterialInterface* BiomePreviewMaterial;
	UPROPERTY(VisibleAnywhere, meta = (Category = "Brush Materials"))
	UMaterialInterface* DebugPaintMaterial;
	
	UPROPERTY(VisibleAnywhere, Transient, meta = (Category = "Debug MIDs"))
	UMaterialInstanceDynamic* DebugDensityMaterial;
	UPROPERTY(EditAnywhere, meta = (Category = "Debug MIDs"))
	UMaterialInterface* DebugMaterial;
	UPROPERTY(EditAnywhere, meta = (Category = "Debug MIDs"))
	UMaterialInstanceDynamic* DebugMID;
	UPROPERTY(VisibleAnywhere, meta = (Category = "Debug RenderTarget"))
	TWeakObjectPtr<UTextureRenderTarget2D> DebugRT;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	TArray<FSpeciesProxy> CreateSpeciesProxy(UBiome* InBiome);
	bool GenerateProceduralContent(bool bPartialUpdate = false, FVector2D DirtyCenter = FVector2D(0, 0),float DirtyRadius = 0);
	void ConvertToFoliageInstance(UBiome* InBiome,const TArray<FScatterPointCloud>& ScatterPointCloud, const FTransform& WorldTM, const float HalfHeight, TArray<FPotentialInstance>& OutFoliageInstances)const;
	void RemoveProceduralContent(bool InRebuildTree = true);
	void CleanPreviousFoliage(const TArray<UFoliageType*>& CleanFoliageTypes,FVector4 DirtyRect);
	UFUNCTION(CallInEditor)
	void RegenerateBiomeData();
	UFUNCTION(CallInEditor)
	void CaptureLandscape();
	void SingleBiomeGeneratePipeline(UBiome* Biome, FBiomeData& InBiomeData, TArray<FPotentialInstance>& OutFoliageInstances,FVector4 DirtyRect);
	void ExcuteBiomeGeneratePipeline(TArray<FPotentialInstance>& OutFoliageInstances, FVector2D EditedCenter, float EditedRadius);
	FVector4 GetLandscapeBound();
	FIntPoint GetLandscapeSize();
	void DrawPreviewBiomeRenderTarget(UTextureRenderTarget2D* RenderTarget,TArray<FLinearColor> PreviewColors);
	TArray<FLinearColor> GetBiomePreviewColor();
};
