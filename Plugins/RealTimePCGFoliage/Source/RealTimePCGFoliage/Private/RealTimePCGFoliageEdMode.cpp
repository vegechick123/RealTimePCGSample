// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealTimePCGFoliageEdMode.h"
#include "RealTimePCGFoliageEdModeToolkit.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMeshActor.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"
#include "LevelEditor.h"
#include "FoliageType.h"
#include "InstancedFoliageActor.h"
#include "Components/ModelComponent.h"
#include "FoliageInstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "Kismet/KismetRenderingLibrary.h"

#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeComponent.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/BrushComponent.h"

const FEditorModeID FRealTimePCGFoliageEdMode::EM_RealTimePCGFoliageEdModeId = TEXT("EM_RealTimePCGFoliageEdMode");
static FName FoliageBrushHighlightColorParamName("HighlightColor");

bool FFoliagePaintingGeometryFilter::operator() (const UPrimitiveComponent* Component) const
{
	if (Component)
	{

		// Whitelist
		bool bAllowed =
			(bAllowLandscape && Component->IsA(ULandscapeHeightfieldCollisionComponent::StaticClass())) ||
			(bAllowStaticMesh && Component->IsA(UStaticMeshComponent::StaticClass()) && !Component->IsA(UFoliageInstancedStaticMeshComponent::StaticClass()))||
				(bAllowBSP && (Component->IsA(UBrushComponent::StaticClass()) || Component->IsA(UModelComponent::StaticClass())));

		// Blacklist
		bAllowed &=
			(bAllowTranslucent || !(Component->GetMaterial(0) && IsTranslucentBlendMode(Component->GetMaterial(0)->GetBlendMode())));

		return bAllowed;
	}

	return false;
}

FRealTimePCGFoliageEdMode::FRealTimePCGFoliageEdMode()
{
	UStaticMesh* StaticMesh = nullptr;
	BrushDefaultHighlightColor = FColor::White;
	if (!IsRunningCommandlet())
	{
		UMaterial* BrushMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EditorLandscapeResources/FoliageBrushSphereMaterial.FoliageBrushSphereMaterial"), nullptr, LOAD_None, nullptr);
		BrushMID = UMaterialInstanceDynamic::Create(BrushMaterial, GetTransientPackage());
		check(BrushMID != nullptr);
		FLinearColor DefaultColor;
		BrushMID->GetVectorParameterDefaultValue(FoliageBrushHighlightColorParamName, DefaultColor);
		BrushDefaultHighlightColor = DefaultColor.ToFColor(false);
		StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/Sphere.Sphere"), nullptr, LOAD_None, nullptr);
	}
	BrushCurrentHighlightColor = BrushDefaultHighlightColor;
	SphereBrushComponent = NewObject<UStaticMeshComponent>(GetTransientPackage(), TEXT("SphereBrushComponent"));
	SphereBrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SphereBrushComponent->SetCollisionObjectType(ECC_WorldDynamic);
	SphereBrushComponent->SetStaticMesh(StaticMesh);
	SphereBrushComponent->SetMaterial(0, BrushMID);
	SphereBrushComponent->SetAbsolute(true, true, true);
	SphereBrushComponent->CastShadow = false;

	bBrushTraceValid = false;
	BrushLocation = FVector::ZeroVector;

	PaintMID = UMaterialInstanceDynamic::Create(LoadObject<UMaterial>(nullptr, TEXT("/RealTimePCGFoliage/Material/M_PaintBrush.M_PaintBrush"), nullptr, LOAD_None, nullptr), GetTransientPackage());

	// Get the default opacity from the material.
	FName OpacityParamName("OpacityAmount");
	BrushMID->GetScalarParameterValue(OpacityParamName, DefaultBrushOpacity);
}

FRealTimePCGFoliageEdMode::~FRealTimePCGFoliageEdMode()
{	
}
/** FGCObject interface */
void FRealTimePCGFoliageEdMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	// Call parent implementation
	FEdMode::AddReferencedObjects(Collector);

	Collector.AddReferencedObject(SphereBrushComponent);
	Collector.AddReferencedObject(PaintMID);
}
void FRealTimePCGFoliageEdMode::Enter()
{
	FEdMode::Enter();

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		Toolkit = MakeShareable(new FRealTimePCGFoliageEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}
	for (TActorIterator<APCGFoliageManager> ActorIter(GetWorld()); ActorIter; ++ActorIter)
	{
		PCGFoliageManager = *ActorIter;		
	}
	PaintMID = UMaterialInstanceDynamic::Create(PCGFoliageManager->PaintMaterial, PCGFoliageManager);

	UE_LOG(LogTemp, Warning, TEXT("The Actor's name is %s"), *PCGFoliageManager->GetName());
	//TArray<AActor*> PCGFoliageManager;
	//UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALandscape::StaticClass(), PCGFoliageManager)
	//TActorIterator<ALandscape> landIterator(GetWorld());
}

void FRealTimePCGFoliageEdMode::Exit()
{
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}
	// Remove the brush
	SphereBrushComponent->UnregisterComponent();
	PCGFoliageManager = nullptr;
	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

bool FRealTimePCGFoliageEdMode::UsesToolkits() const
{
	return true;
}

void FRealTimePCGFoliageEdMode::PostUndo()
{
	FEdMode::PostUndo();

	//PopulateFoliageMeshList();
}

void FRealTimePCGFoliageEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	//if (!IsEditingEnabled())
	//{
	//	return;
	//}

	if (bToolActive)
	{
		ApplyBrush(ViewportClient);
	}

	FEdMode::Tick(ViewportClient, DeltaTime);

	//if (UISettings.GetSelectToolSelected() || UISettings.GetLassoSelectToolSelected())
	//{
	//	// Update pivot
	//	UpdateWidgetLocationToInstanceSelection();
	//}

	// Update the position and size of the brush component
	if (bBrushTraceValid)
	{
		// Scale adjustment is due to default sphere SM size.
		FTransform BrushTransform = FTransform(FQuat::Identity, BrushLocation, FVector(GetPaintingBrushRadius() * 0.00625f));
		SphereBrushComponent->SetRelativeTransform(BrushTransform);

		static FColor BrushSingleInstanceModeHighlightColor = FColor::Green;
		FColor BrushHighlightColor =  BrushDefaultHighlightColor;
		if (BrushCurrentHighlightColor != BrushHighlightColor)
		{
			BrushCurrentHighlightColor = BrushHighlightColor;
			BrushMID->SetVectorParameterValue(FoliageBrushHighlightColorParamName, BrushHighlightColor);
		}

		if (!SphereBrushComponent->IsRegistered())
		{
			SphereBrushComponent->RegisterComponentWithWorld(ViewportClient->GetWorld());
		}
	}
	else
	{
		if (SphereBrushComponent->IsRegistered())
		{
			SphereBrushComponent->UnregisterComponent();
		}
	}
}
void FRealTimePCGFoliageEdMode::StartFoliageBrushTrace(FEditorViewportClient* ViewportClient, class UViewportInteractor* Interactor)
{
	if (!bToolActive)
	{
		GEditor->BeginTransaction(NSLOCTEXT("UnrealEd", "FoliageMode_EditTransaction", "Foliage Editing"));
		PreApplyBrush();
		ApplyBrush(ViewportClient);

		bToolActive = true;		
	}
}

void FRealTimePCGFoliageEdMode::EndFoliageBrushTrace()
{
	GEditor->EndTransaction();
	bToolActive = false;
	bBrushTraceValid = false;
}
/** Trace and update brush position */
void FRealTimePCGFoliageEdMode::FoliageBrushTrace(FEditorViewportClient* ViewportClient, const FVector& InRayOrigin, const FVector& InRayDirection)
{
	bBrushTraceValid = false;
	if (ViewportClient == nullptr || (!ViewportClient->IsMovingCamera() && ViewportClient->IsVisible()))
	{
		if (true)
		{
			const FVector TraceStart(InRayOrigin);
			const FVector TraceEnd(InRayOrigin + InRayDirection * HALF_WORLD_MAX);

			FHitResult Hit;
			UWorld* World = GetWorld();
			static FName NAME_FoliageBrush = FName(TEXT("FoliageBrush"));
			FFoliagePaintingGeometryFilter FilterFunc;

			if (AInstancedFoliageActor::FoliageTrace(World, Hit, FDesiredFoliageInstance(TraceStart, TraceEnd), NAME_FoliageBrush, false))
			{
				UPrimitiveComponent* PrimComp = Hit.Component.Get();
				if (PrimComp != nullptr)
				{
					if (!bAdjustBrushRadius)
					{
						// Adjust the brush location
						BrushLocation = Hit.Location;
						BrushNormal = Hit.Normal;
					}

					// Still want to draw the brush when resizing
					bBrushTraceValid = true;
				}
			}
		}
	}
}


/**
* Called when the mouse is moved over the viewport
*
* @param	InViewportClient	Level editor viewport client that captured the mouse input
* @param	InViewport			Viewport that captured the mouse input
* @param	InMouseX			New mouse cursor X coordinate
* @param	InMouseY			New mouse cursor Y coordinate
*
* @return	true if input was handled
*/
bool FRealTimePCGFoliageEdMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY)
{
	// Compute a world space ray from the screen space mouse coordinates
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		ViewportClient->Viewport,
		ViewportClient->GetScene(),
		ViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(ViewportClient->IsRealtime()));

	FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
	FViewportCursorLocation MouseViewportRay(View, ViewportClient, MouseX, MouseY);
	BrushTraceDirection = MouseViewportRay.GetDirection();

	FVector BrushTraceStart = MouseViewportRay.GetOrigin();
	if (ViewportClient->IsOrtho())
	{
		BrushTraceStart += -WORLD_MAX * BrushTraceDirection;
	}

	FoliageBrushTrace(ViewportClient, BrushTraceStart, BrushTraceDirection);
	return false;
}
/**
* Called when the mouse is moved while a window input capture is in effect
*
* @param	InViewportClient	Level editor viewport client that captured the mouse input
* @param	InViewport			Viewport that captured the mouse input
* @param	InMouseX			New mouse cursor X coordinate
* @param	InMouseY			New mouse cursor Y coordinate
*
* @return	true if input was handled
*/
bool FRealTimePCGFoliageEdMode::CapturedMouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY)
{
	//Compute a world space ray from the screen space mouse coordinates
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		ViewportClient->Viewport,
		ViewportClient->GetScene(),
		ViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(ViewportClient->IsRealtime()));

	FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
	FViewportCursorLocation MouseViewportRay(View, ViewportClient, MouseX, MouseY);
	BrushTraceDirection = MouseViewportRay.GetDirection();

	FVector BrushTraceStart = MouseViewportRay.GetOrigin();
	if (ViewportClient->IsOrtho())
	{
		BrushTraceStart += -WORLD_MAX * BrushTraceDirection;
	}

	FoliageBrushTrace(ViewportClient, BrushTraceStart, BrushTraceDirection);
	return false;
}
/** FEdMode: Called when a mouse button is pressed */
bool FRealTimePCGFoliageEdMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	bTracking = true;
	if (IsCtrlDown(InViewport) && InViewport->KeyState(EKeys::MiddleMouseButton) && (UISettings.GetPaintToolSelected()))
	{
		bAdjustBrushRadius = true;
	}
	else
	{
		bTracking = false;
		return FEdMode::StartTracking(InViewportClient, InViewport);
	}

	return true;
}

bool FRealTimePCGFoliageEdMode::EndTracking()
{
	bTracking = false;

	if (bAdjustBrushRadius)
	{
		bAdjustBrushRadius = false;
		return true;
	}
	return false;
}

/** FEdMode: Called when the a mouse button is released */
bool FRealTimePCGFoliageEdMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (EndTracking())
	{
		return true;
	}

	return FEdMode::EndTracking(InViewportClient, InViewport);
}
/** FEdMode: Called when a key is pressed */
bool FRealTimePCGFoliageEdMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{

	bool bHandled = false;
	if (UISettings.GetPaintToolSelected())
	{
		// Require Ctrl or not as per user preference
		ELandscapeFoliageEditorControlType FoliageEditorControlType = GetDefault<ULevelEditorViewportSettings>()->FoliageEditorControlType;

		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			// Only activate tool if we're not already moving the camera and we're not trying to drag a transform widget
			// Not using "if (!ViewportClient->IsMovingCamera())" because it's wrong in ortho viewports :D
			bool bMovingCamera = Viewport->KeyState(EKeys::MiddleMouseButton) || Viewport->KeyState(EKeys::RightMouseButton) || IsAltDown(Viewport);

			if ((Viewport->IsPenActive() && Viewport->GetTabletPressure() > 0.f) ||
				(!bMovingCamera && ViewportClient->GetCurrentWidgetAxis() == EAxisList::None &&
					(FoliageEditorControlType == ELandscapeFoliageEditorControlType::IgnoreCtrl ||
						(FoliageEditorControlType == ELandscapeFoliageEditorControlType::RequireCtrl && IsCtrlDown(Viewport)) ||
						(FoliageEditorControlType == ELandscapeFoliageEditorControlType::RequireNoCtrl && !IsCtrlDown(Viewport)))))
			{
				if (!bToolActive)
				{
					StartFoliageBrushTrace(ViewportClient);

					bHandled = true;
				}
			}
		}
		else if (bToolActive && Event == IE_Released &&
			(Key == EKeys::LeftMouseButton || (FoliageEditorControlType == ELandscapeFoliageEditorControlType::RequireCtrl && (Key == EKeys::LeftControl || Key == EKeys::RightControl))))
		{
			//Set the cursor position to that of the slate cursor so it wont snap back
			Viewport->SetPreCaptureMousePosFromSlateCursor();
			EndFoliageBrushTrace();

			bHandled = true;
		}


		if (IsCtrlDown(Viewport))
		{
			// Control + scroll adjusts the brush radius
			static const float RadiusAdjustmentAmount = 25.f;
			if (Key == EKeys::MouseScrollUp)
			{
				AdjustBrushRadius(RadiusAdjustmentAmount);

				bHandled = true;
			}
			else if (Key == EKeys::MouseScrollDown)
			{
				AdjustBrushRadius(-RadiusAdjustmentAmount);

				bHandled = true;
			}
		}
	}


	return bHandled;
}
/** FEdMode: Called when mouse drag input it applied */
bool FRealTimePCGFoliageEdMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (bAdjustBrushRadius)
	{
		if (UISettings.GetPaintToolSelected())
		{
			static const float RadiusAdjustmentFactor = 10.f;
			AdjustBrushRadius(RadiusAdjustmentFactor * InDrag.Y);

			return true;
		}
	}
	return FEdMode::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
}

/** FEdMode: Render the foliage edit mode */
void FRealTimePCGFoliageEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	/** Call parent implementation */
	FEdMode::Render(View, Viewport, PDI);
}


/** FEdMode: Render HUD elements for this tool */
void FRealTimePCGFoliageEdMode::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
}
bool FRealTimePCGFoliageEdMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	return FEdMode::HandleClick(InViewportClient, HitProxy, Click);
}
void FRealTimePCGFoliageEdMode::PreApplyBrush()
{

}
void FRealTimePCGFoliageEdMode::ApplyBrush(FEditorViewportClient* ViewportClient)
{
	ALandscape* Land = GetLandscape();
	FVector LocalCoord = Land->GetTransform().InverseTransformPosition(BrushLocation);
	LocalCoord.X /= 505;
	LocalCoord.Y /= 505;
	UCanvas* Canvas = nullptr;
	FVector2D Size;
	FDrawToRenderTargetContext Context;
	SetPaintMaterial();
	//UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(PCGFoliageManager,GetEditedRT(),Canvas,Size,Context);
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(PCGFoliageManager, GetEditedRT(), PaintMID);

	//UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(PCGFoliageManager, Context);
	
}

void FRealTimePCGFoliageEdMode::AdjustBrushRadius(float Multiplier)
{
	const float PercentageChange = 0.05f;
	const float CurrentBrushRadius = UISettings.GetRadius();

	float NewValue = CurrentBrushRadius * (1 + PercentageChange * Multiplier);
	UISettings.SetRadius(FMath::Clamp(NewValue, 0.1f, 8192.0f));
}

void FRealTimePCGFoliageEdMode::SetBrushOpacity(const float InOpacity)
{
	static FName OpacityParamName("OpacityAmount");
	BrushMID->SetScalarParameterValue(OpacityParamName, InOpacity);
}
float FRealTimePCGFoliageEdMode::GetPaintingBrushRadius() const
{
	float Radius = UISettings.GetRadius();
	return Radius;
}

ALandscape* FRealTimePCGFoliageEdMode::GetLandscape() const
{
	return PCGFoliageManager->Landscape;
}

UTextureRenderTarget2D* FRealTimePCGFoliageEdMode::GetEditedRT() const
{
	return PCGFoliageManager->Mask;
}

void FRealTimePCGFoliageEdMode::SetPaintMaterial()
{
	ALandscape* Land = GetLandscape();
	;
	FVector LocalCoord = Land->GetTransform().InverseTransformPosition(BrushLocation);
	LocalCoord.X /= 505;
	LocalCoord.Y /= 505;
	PaintMID->SetScalarParameterValue("Radius", UISettings.GetRadius()/505/100);
	PaintMID->SetVectorParameterValue("Center", LocalCoord);
	PaintMID->SetVectorParameterValue("Color", FLinearColor::Red);
	UE_LOG(LogTemp, Warning, TEXT("Radius %f"), UISettings.GetRadius() / 505/100);
	//UE_LOG(LogTemp, Warning, TEXT("Hit %f %f %f"), BrushLocation.X, BrushLocation.Y, BrushLocation.Z);
}


void FRealTimePCGFoliageEdMode::AddInstances(UWorld* InWorld, const TArray<FDesiredFoliageInstance>& DesiredInstances, const FFoliagePaintingGeometryFilter& OverrideGeometryFilter, bool InRebuildFoliageTree)
{
	TMap<const UFoliageType*, TArray<FDesiredFoliageInstance>> SettingsInstancesMap;
	for (const FDesiredFoliageInstance& DesiredInst : DesiredInstances)
	{
		TArray<FDesiredFoliageInstance>& Instances = SettingsInstancesMap.FindOrAdd(DesiredInst.FoliageType);
		Instances.Add(DesiredInst);
	}

	for (auto It = SettingsInstancesMap.CreateConstIterator(); It; ++It)
	{
		const UFoliageType* FoliageType = It.Key();

		const TArray<FDesiredFoliageInstance>& Instances = It.Value();
		AddInstancesImp(InWorld, FoliageType, Instances, TArray<int32>(), 1.f, nullptr, nullptr, &OverrideGeometryFilter, InRebuildFoliageTree);
	}
}


bool FRealTimePCGFoliageEdMode::AddInstancesImp(UWorld* InWorld,UFoliageType* Settings, const TArray<FDesiredFoliageInstance>& DesiredInstances, const TArray<int32>& ExistingInstanceBuckets, const float Pressure, LandscapeLayerCacheData* LandscapeLayerCachesPtr, const FFoliageUISettings* UISettings, const FFoliagePaintingGeometryFilter* OverrideGeometryFilter, bool InRebuildFoliageTree)
{

	if (DesiredInstances.Num() == 0)
	{
		return false;
	}

	TArray<FPotentialInstance> PotentialInstanceBuckets[NUM_INSTANCE_BUCKETS];
	if (DesiredInstances[0].PlacementMode == EFoliagePlacementMode::Manual)
	{
		CalculatePotentialInstances(InWorld, Settings, DesiredInstances, PotentialInstanceBuckets, LandscapeLayerCachesPtr, UISettings, OverrideGeometryFilter);
	}
	else
	{
		//@TODO: actual threaded part coming, need parts of this refactor sooner for content team
		CalculatePotentialInstances_ThreadSafe(InWorld, Settings, &DesiredInstances, PotentialInstanceBuckets, nullptr, 0, DesiredInstances.Num() - 1, OverrideGeometryFilter);

		// Existing foliage types in the palette  we want to override any existing mesh settings with the procedural settings.
		TMap<AInstancedFoliageActor*, TArray<const UFoliageType*>> UpdatedTypesByIFA;
		for (TArray<FPotentialInstance>& Bucket : PotentialInstanceBuckets)
		{
			for (auto& PotentialInst : Bucket)
			{
				// Get the IFA for the base component level that contains the component the instance will be placed upon
				AInstancedFoliageActor* TargetIFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(PotentialInst.HitComponent->GetComponentLevel(), true);

				// Update the type in the IFA if needed
				TArray<const UFoliageType*>& UpdatedTypes = UpdatedTypesByIFA.FindOrAdd(TargetIFA);
				if (!UpdatedTypes.Contains(PotentialInst.DesiredInstance.FoliageType))
				{
					UpdatedTypes.Add(PotentialInst.DesiredInstance.FoliageType);
					TargetIFA->AddFoliageType(PotentialInst.DesiredInstance.FoliageType);
				}
			}
		}
	}

	bool bPlacedInstances = false;

	for (int32 BucketIdx = 0; BucketIdx < NUM_INSTANCE_BUCKETS; BucketIdx++)
	{
		TArray<FPotentialInstance>& PotentialInstances = PotentialInstanceBuckets[BucketIdx];
		float BucketFraction = (float)(BucketIdx + 1) / (float)NUM_INSTANCE_BUCKETS;

		// We use the number that actually succeeded in placement (due to parameters) as the target
		// for the number that should be in the brush region.
		const int32 BucketOffset = (ExistingInstanceBuckets.Num() ? ExistingInstanceBuckets[BucketIdx] : 0);
		int32 AdditionalInstances = FMath::Clamp<int32>(FMath::RoundToInt(BucketFraction * (float)(PotentialInstances.Num() - BucketOffset) * Pressure), 0, PotentialInstances.Num());

		{

			TArray<FFoliageInstance> PlacedInstances;
			PlacedInstances.Reserve(AdditionalInstances);

			for (int32 Idx = 0; Idx < AdditionalInstances; Idx++)
			{
				FPotentialInstance& PotentialInstance = PotentialInstances[Idx];
				FFoliageInstance Inst;
				if (PotentialInstance.PlaceInstance(InWorld, Settings, Inst))
				{
					Inst.ProceduralGuid = PotentialInstance.DesiredInstance.ProceduralGuid;
					Inst.BaseComponent = PotentialInstance.HitComponent;
					PlacedInstances.Add(MoveTemp(Inst));
					bPlacedInstances = true;
				}
			}

			SpawnFoliageInstance(InWorld, Settings, PlacedInstances);
		}
	}

	return bPlacedInstances;
}

void FRealTimePCGFoliageEdMode::CalculatePotentialInstances_ThreadSafe(const UWorld* InWorld, const UFoliageType* Settings, const TArray<FDesiredFoliageInstance>* DesiredInstances, TArray<FPotentialInstance> OutPotentialInstances[NUM_INSTANCE_BUCKETS], const FFoliageUISettings* UISettings, const int32 StartIdx, const int32 LastIdx, const FFoliagePaintingGeometryFilter* OverrideGeometryFilter)
{

	// Reserve space in buckets for a potential instances 
	for (int32 BucketIdx = 0; BucketIdx < NUM_INSTANCE_BUCKETS; ++BucketIdx)
	{
		auto& Bucket = OutPotentialInstances[BucketIdx];
		Bucket.Reserve(DesiredInstances->Num());
	}

	for (int32 InstanceIdx = StartIdx; InstanceIdx <= LastIdx; ++InstanceIdx)
	{
		const FDesiredFoliageInstance& DesiredInst = (*DesiredInstances)[InstanceIdx];
		FHitResult Hit;
		static FName NAME_AddFoliageInstances = FName(TEXT("AddFoliageInstances"));

		FFoliageTraceFilterFunc TraceFilterFunc;
		if (DesiredInst.PlacementMode == EFoliagePlacementMode::Manual)
		{
			// Enable geometry filters when painting foliage manually
			TraceFilterFunc = FFoliagePaintingGeometryFilter();
		}

		if (OverrideGeometryFilter)
		{
			TraceFilterFunc = *OverrideGeometryFilter;
		}

		if (AInstancedFoliageActor::FoliageTrace(InWorld, Hit, DesiredInst, NAME_AddFoliageInstances, true, TraceFilterFunc))
		{
			float HitWeight = 1.f;
			const int32 BucketIndex = FMath::RoundToInt(HitWeight * (float)(NUM_INSTANCE_BUCKETS - 1));
			OutPotentialInstances[BucketIndex].Add(FPotentialInstance(Hit.ImpactPoint, Hit.ImpactNormal, Hit.Component.Get(), HitWeight, DesiredInst));

		}
	}
}

static void SpawnFoliageInstance(UWorld* InWorld, UFoliageType* Settings, const TArray<FFoliageInstance>& PlacedInstances)
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(InWorld, true);
	FFoliageInfo* FoliageInfo = IFA->FindOrAddMesh(Settings);
	if (!FoliageInfo)
		return;

	TArray<const FFoliageInstance*> FoliageInstancePointers;
	for (const FFoliageInstance& Instance : PlacedInstances)
	{
		FoliageInstancePointers.Add(&Instance);
	}

	FoliageInfo->AddInstances(IFA, Settings, FoliageInstancePointers);

	IFA->RegisterAllComponents();
}