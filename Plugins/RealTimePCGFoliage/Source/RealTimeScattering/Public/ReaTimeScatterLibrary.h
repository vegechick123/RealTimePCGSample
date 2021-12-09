// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReaTimeScatterLibrary.generated.h"
/**
 * 
 */
struct FScatterPoint
{
	float LocationX;
	float LocationY;
	FVector2D Location()const
	{
		return FVector2D(LocationX, LocationY);
	}
};
USTRUCT(BlueprintType)
struct FScatterPattern
{
	GENERATED_BODY()	
	TArray<FVector2D> PointCloud;
	FVector2D Size;
};

USTRUCT(BlueprintType)
struct FScatterPointCloud
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	TArray<FVector2D> Points;
	TArray<FScatterPoint> ScatterPoints;
};
USTRUCT(BlueprintType)
struct REALTIMESCATTERING_API FSpeciesProxy
{
	GENERATED_BODY()
	float Radius;
	float Ratio;
};

UCLASS()
class REALTIMESCATTERING_API UReaTimeScatterLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public :
	UFUNCTION(BlueprintCallable)
		static FScatterPattern GetDefaultPattern()
	{
		FScatterPattern Result;
		Result.PointCloud.Add(FVector2D(299.100372314, 94.2850341797));
		Result.PointCloud.Add(FVector2D(563.941162109, 426.259460449));
		Result.PointCloud.Add(FVector2D(250.259140015, 731.499572754));
		Result.PointCloud.Add(FVector2D(768.820617676, 433.105560303));
		Result.PointCloud.Add(FVector2D(123.064186096, 124.732849121));
		Result.PointCloud.Add(FVector2D(788.588500977, 321.672210693));
		Result.PointCloud.Add(FVector2D(539.369995117, 320.49029541));
		Result.PointCloud.Add(FVector2D(162.273880005, 242.290039062));
		Result.PointCloud.Add(FVector2D(63.9281768799, 222.246261597));
		Result.PointCloud.Add(FVector2D(349.455413818, 718.702026367));
		Result.PointCloud.Add(FVector2D(576.486877441, 111.798477173));
		Result.PointCloud.Add(FVector2D(182.860565186, 444.683685303));
		Result.PointCloud.Add(FVector2D(48.6409683228, 16.2424316406));
		Result.PointCloud.Add(FVector2D(20.8261966705, 114.433456421));
		Result.PointCloud.Add(FVector2D(615.319335938, 16.3578987122));
		Result.PointCloud.Add(FVector2D(107.576797485, 630.393310547));
		Result.PointCloud.Add(FVector2D(87.520149231, 532.360961914));
		Result.PointCloud.Add(FVector2D(501.581756592, 178.759414673));
		Result.PointCloud.Add(FVector2D(652.06060791, 691.998962402));
		Result.PointCloud.Add(FVector2D(191.668197632, 25.4294624329));
		Result.PointCloud.Add(FVector2D(262.537139893, 251.08694458));
		Result.PointCloud.Add(FVector2D(150.034927368, 721.16394043));
		Result.PointCloud.Add(FVector2D(73.7247467041, 433.296722412));
		Result.PointCloud.Add(FVector2D(344.359741211, 185.548538208));
		Result.PointCloud.Add(FVector2D(220.077850342, 160.084533691));
		Result.PointCloud.Add(FVector2D(291.227172852, 361.178405762));
		Result.PointCloud.Add(FVector2D(208.991210938, 640.332336426));
		Result.PointCloud.Add(FVector2D(454.791259766, 267.137420654));
		Result.PointCloud.Add(FVector2D(467.73248291, 453.533691406));
		Result.PointCloud.Add(FVector2D(433.826873779, 772.419189453));
		Result.PointCloud.Add(FVector2D(94.1911010742, 335.411437988));
		Result.PointCloud.Add(FVector2D(282.824645996, 460.824768066));
		Result.PointCloud.Add(FVector2D(354.798950195, 530.633850098));
		Result.PointCloud.Add(FVector2D(29.2692661285, 692.633605957));
		Result.PointCloud.Add(FVector2D(194.177505493, 337.065185547));
		Result.PointCloud.Add(FVector2D(452.550689697, 552.374694824));
		Result.PointCloud.Add(FVector2D(684.537902832, 285.854034424));
		Result.PointCloud.Add(FVector2D(373.783935547, 419.274719238));
		Result.PointCloud.Add(FVector2D(420.446716309, 120.303367615));
		Result.PointCloud.Add(FVector2D(508.510314941, 21.8121051788));
		Result.PointCloud.Add(FVector2D(549.304199219, 578.881469727));
		Result.PointCloud.Add(FVector2D(734.299804688, 748.891113281));
		Result.PointCloud.Add(FVector2D(760.079589844, 173.802597046));
		Result.PointCloud.Add(FVector2D(658.208618164, 592.188171387));
		Result.PointCloud.Add(FVector2D(356.469970703, 285.391174316));
		Result.PointCloud.Add(FVector2D(637.091552734, 494.443237305));
		Result.PointCloud.Add(FVector2D(310.234527588, 620.283996582));
		Result.PointCloud.Add(FVector2D(223.607940674, 541.40637207));
		Result.PointCloud.Add(FVector2D(552.208312988, 686.471008301));
		Result.PointCloud.Add(FVector2D(594.380371094, 216.021011353));
		Result.PointCloud.Add(FVector2D(418.842926025, 646.616943359));
		Result.PointCloud.Add(FVector2D(766.980102539, 567.164489746));


		Result.Size.X = Result.Size.Y = 800;
		return Result;
	}
	UFUNCTION(BlueprintCallable)
	static void RealTImeScatter(const TArray<FColor>& ColorData, FIntPoint TextureSize, FVector2D BottomLeft, FVector2D TopRight, const FScatterPattern& Pattern, TArray<FVector2D>& Result, float RadiusScale, float Ratio, bool FlipY);
	UFUNCTION(BlueprintCallable)
	static void RealTImeScatterGPU(UObject* WorldContextObject, TArray<UTextureRenderTarget2D*> DensityMaps, UTextureRenderTarget2D* OutputDistanceField,FVector2D BottomLeft, FVector2D TopRight, const FScatterPattern& Pattern,const TArray<FSpeciesProxy>& InData,	TArray<FScatterPointCloud>& Result ,bool FlipY);
	UFUNCTION(BlueprintCallable)
	static void ScatterWithCollision(UObject* WorldContextObject,TArray<UTextureRenderTarget2D*> DensityMaps,UTextureRenderTarget2D* OutputDistanceField,FVector2D BottomLeft, FVector2D TopRight, const FScatterPattern& Pattern, const TArray<FSpeciesProxy>& InData, TArray<FScatterPointCloud>& OutData, bool FlipY,bool UseGPU);	
	UFUNCTION(BlueprintCallable)
	static void JumpFlood(UObject* WorldContextObject, UTextureRenderTarget2D* InputSeed, UTextureRenderTarget2D* InputStepRT, UTextureRenderTarget2D* OutputStepRT, UTextureRenderTarget2D* OutputSDF,float LengthScale);
};
