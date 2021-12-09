// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget.h"
#include "TextureCollectComponentBase.generated.h"


UCLASS(Blueprintable, ClassGroup = "Custom", meta = (BlueprintSpawnableComponent))
class REALTIMEPCGFOLIAGE_API UTextureCollectComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTextureCollectComponentBase();
	UFUNCTION(BlueprintImplementableEvent)
	void RenderDensityTexture(UTextureRenderTarget2D* InPlacement, UTextureRenderTarget2D* OutDensity,UMaterialInterface* DensityMaterial);
	UFUNCTION(BlueprintImplementableEvent)
	void SetUpMID(UMaterialInstanceDynamic* DensityMID);
protected:


		
};
