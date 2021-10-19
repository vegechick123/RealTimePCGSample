// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGFoliageManager.h"
#include "RealTimePCGFoliageEdMode.h"
// Sets default values
APCGFoliageManager::APCGFoliageManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
		ReBuild = false;
		GenerateProceduralContent();
	}
}

bool APCGFoliageManager::GenerateProceduralContent()
{
#if WITH_EDITOR
	TArray<FDesiredFoliageInstance> OutFoliageInstances;

	bool Result = ExecuteSimulation(OutFoliageInstances);
	FRealTimePCGFoliageEdMode::AddInstances(GetWorld(),OutFoliageInstances);
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
	//Landscape->
	FVector Scale = Landscape->GetTransform().GetScale3D();
	FVector2D Size = FVector2D(Scale.X, Scale.Y);
	TArray<FScatterPointCloud> ScatterPointCloud;
	TArray<FDesiredFoliageInstance> OutInstances;
	UReaTimeScatterLibrary::ScatterWithCollision(this,Mask,Size/2, - Size / 2,Pattern,CollisionPasses, ScatterPointCloud,false,true);
	FTransform WorldTM;
	ConvertToFoliageInstance(ScatterPointCloud,WorldTM,2000,OutFoliageInstances);
	
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
			//DesiredInst->ProceduralGuid = ProceduralGuid;
			DesiredInst->FoliageType = PCGFoliageTypes[i].FoliageType;
			//DesiredInst->Age = 1f;
			//DesiredInst->ProceduralVolumeBodyInstance = VolumeBodyInstance;
			DesiredInst->PlacementMode = EFoliagePlacementMode::Procedural;
		}
	}
}

void APCGFoliageManager::RemoveProceduralContent(bool InRebuildTree)
{
}

