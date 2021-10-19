// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Landscape.h"
#include "InstancedFoliage.h"
#include "ReaTimeScatterLibrary.h"

#include "PCGFoliageManager.generated.h"

USTRUCT(BlueprintType)
struct REALTIMEPCGFOLIAGE_API FPCGFoliageType
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	UFoliageType* FoliageType;
	UPROPERTY(EditAnywhere)
	FCollisionPass CollisionPass;
};

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
	ALandscape* Landscape;
	UPROPERTY(EditAnywhere)
	UTextureRenderTarget2D* Mask;
	UPROPERTY(EditAnywhere)
	UMaterialInterface* PaintMaterial;

	UPROPERTY(EditAnywhere)
	TArray<FPCGFoliageType> PCGFoliageTypes;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
		ReBuild = false;
	}
	/**
	 * Runs the procedural foliage simulation to generate a list of desired instances to spawn.
	 * @return True if the simulation succeeded
	 */
	TArray<FCollisionPass> GetCollisionPasses();
	bool GenerateProceduralContent(TArray<FDesiredFoliageInstance>& OutFoliageInstances);
	bool ExecuteSimulation(TArray<FDesiredFoliageInstance>& OutFoliageInstances);
	void ConvertToFoliageInstance(const TArray<FScatterPointCloud>& ScatterPointCloud, const FTransform& WorldTM, const float HalfHeight, TArray<FDesiredFoliageInstance>& OutFoliageInstances)const;
	void RemoveProceduralContent(bool InRebuildTree = true);
};
