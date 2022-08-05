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
#include "RealTimePCGUtils.h"
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
	PaintRTCache = nullptr;
	BiomePreviewRenderTarget = nullptr;
	bBrushTraceValid = false;
	BrushLocation = FVector::ZeroVector;

	PaintMID = UMaterialInstanceDynamic::Create(LoadObject<UMaterial>(nullptr, TEXT("/RealTimePCGFoliage/Material/M_PaintBrush.M_PaintBrush"), nullptr, LOAD_None, nullptr), GetTransientPackage());
	BiomePreviewRenderTarget = RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(nullptr,"BiomePreviewRenderTarget",FIntPoint(128,128),ETextureRenderTargetFormat::RTF_RGBA8,FLinearColor::Black);
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
	Collector.AddReferencedObject(BiomePreviewRenderTarget);
	if(PaintRTCache)
		Collector.AddReferencedObject(PaintRTCache);
}
void FRealTimePCGFoliageEdMode::Enter()
{
	FEdMode::Enter();
	UISettings.Load();
	// Clear any selection in case the instanced foliage actor is selected
	GEditor->SelectNone(true, true);

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		Toolkit = MakeShareable(new FRealTimePCGFoliageEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());

		UICommandList = Toolkit->GetToolkitCommands();
		BindCommands();
	}
	for (TActorIterator<APCGFoliageManager> ActorIter(GetWorld()); ActorIter; ++ActorIter)
	{
		PCGFoliageManager = *ActorIter;		
	}	
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
	UISettings.Save();
	// Remove the brush
	SphereBrushComponent->UnregisterComponent();
	PCGFoliageManager = nullptr;
	//PaintMID->ClearParameterValues();
	//PaintRTCache.R
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
		if ((Key == EKeys::LeftShift || Key == EKeys::RightShift) && Event == IE_Released)
		{
			UISettings.IsInQuickEraseMode=false;
		}
		else if ((Key == EKeys::LeftShift || Key == EKeys::RightShift) && Event == IE_Pressed)
		{
			UISettings.IsInQuickEraseMode = true;
		}

		if (IsCtrlDown(Viewport))
		{

			if (Key == EKeys::MouseScrollUp)
			{
				AdjustBrushRadius(2);

				bHandled = true;
			}
			else if (Key == EKeys::MouseScrollDown)
			{
				AdjustBrushRadius(0.5f);

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
bool FRealTimePCGFoliageEdMode::IsSelectionAllowed(AActor* InActor, bool bInSelection) const
{
	return false;
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
	SetPaintMaterial();
	const FScopedTransaction Transaction(FText::FromString("Draw Mask"));
	FVector4 DirtyRect = FVector4(BrushLocation.X - GetPaintingBrushRadius(), BrushLocation.Y - GetPaintingBrushRadius(), BrushLocation.X+GetPaintingBrushRadius(), BrushLocation.Y + GetPaintingBrushRadius());
	PaintRTCache = RealTimePCGUtils::GetOrCreateTransientRenderTarget2D(PaintRTCache, "PaintRTCache", PCGFoliageManager->TextureSize, RTF_R8,FLinearColor::Black);	
	double start, end;
	start = FPlatformTime::Seconds();

	
	if (UISettings.bPaintSpecies)
	{
		
		UTexture2D* PaintTexture = GetEditedSpeciesCleanTexture();
		if (!PaintTexture)
			return;
		PCGFoliageManager->DebugPaintMaterial = PaintMID;
		//PaintMID->SetVectorParameterValue("Color", FLinearColor::Red);
		PaintMID->SetTextureParameterValue("Texture", PaintTexture);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(PCGFoliageManager.Get(), PaintRTCache, PaintMID);

		FlushRenderingCommands();
		CopyRenderTargetToTexture(PaintTexture, PaintRTCache);
	}
	else
	{
		FBiomeData* EditedBiomeData = GetEditedBiomeData();
		if (!EditedBiomeData)
			return;
		UTexture2D* PaintTexture = EditedBiomeData->PlacementMap;
		for (FBiomeData& BiomeData : PCGFoliageManager->BiomeData)
		{
			if (PaintTexture == BiomeData.PlacementMap)
			{
				PaintMID->SetVectorParameterValue("Color", GetErase()?FLinearColor::Black:FLinearColor::Red);
			}
			else
			{
				PaintMID->SetVectorParameterValue("Color", FLinearColor::Black);
			}
			PaintMID->SetTextureParameterValue("Texture", BiomeData.PlacementMap);
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(PCGFoliageManager.Get(), PaintRTCache, PaintMID);

			FlushRenderingCommands();
			CopyRenderTargetToTexture(BiomeData.PlacementMap, PaintRTCache);
		}
		
	}
	end = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Warning, TEXT("PaintTexture in %f seconds."), end - start);
	TArray<FLinearColor> PreviewColor= PCGFoliageManager->GetBiomePreviewColor();

	PCGFoliageManager->DrawPreviewBiomeRenderTarget(BiomePreviewRenderTarget, PreviewColor);
	PCGFoliageManager->DebugRT = BiomePreviewRenderTarget;
	PCGFoliageManager->DebugPaintMaterial = PaintMID;
	PCGFoliageManager->Modify();
	PCGFoliageManager->GenerateProceduralContent(true,FVector2D(BrushLocation.X, BrushLocation.Y), GetPaintingBrushRadius());
	
}

void FRealTimePCGFoliageEdMode::AdjustBrushRadius(float Multiplier)
{

	const float CurrentBrushRadius = UISettings.GetRadius();

	float NewValue = CurrentBrushRadius *  Multiplier;
	UISettings.SetRadius(FMath::Clamp(NewValue, 0.1f, 8192.0f));
}

void FRealTimePCGFoliageEdMode::SetBrushOpacity(const float InOpacity)
{
	static FName OpacityParamName("OpacityAmount");
	BrushMID->SetScalarParameterValue(OpacityParamName, InOpacity);
}
void FRealTimePCGFoliageEdMode::SetPaintBiome()
{
	UISettings.bPaintSpecies = false;
	UISettings.bPaintBiome = true;
}
void FRealTimePCGFoliageEdMode::SetPaintSpecies()
{
	UISettings.bPaintBiome = false;
	UISettings.bPaintSpecies = true;	
}

bool FRealTimePCGFoliageEdMode::GetPaintBiome()const
{
	return UISettings.bPaintBiome;
}

bool FRealTimePCGFoliageEdMode::GetPaintSpecies()const
{
	return UISettings.bPaintSpecies;
}

bool FRealTimePCGFoliageEdMode::GetErase() const
{
	return UISettings.IsInQuickEraseMode;
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

UTexture2D*  FRealTimePCGFoliageEdMode::GetEditedTexture()
{
	//(FRealTimePCGFoliageEdModeToolkit*)(Toolkit.Get())->;
	if (GetPaintSpecies())
	{
		return GetEditedSpeciesCleanTexture();
	}
	else
	{
		return GetEditedBiomeData()->PlacementMap;
	}
	
}

void FRealTimePCGFoliageEdMode::SetPaintMaterial()
{
	ALandscape* Land = GetLandscape();
	FVector LocalCoord = Land->GetTransform().InverseTransformPosition(BrushLocation);
	FIntPoint Resolution = PCGFoliageManager->GetLandscapeSize();

	LocalCoord.X /= Resolution.X;
	LocalCoord.Y /= Resolution.Y;
	PaintMID->SetScalarParameterValue("Radius", UISettings.GetRadius()/ Resolution.X /100);
	PaintMID->SetVectorParameterValue("Center", LocalCoord);
	if(GetErase())
		PaintMID->SetVectorParameterValue("Color", FLinearColor::Black);
	else
		PaintMID->SetVectorParameterValue("Color", FLinearColor::Red);

	if (GetPaintSpecies())
		PaintMID->SetScalarParameterValue("FallOff", 1);
	else
		PaintMID->SetScalarParameterValue("FallOff", 0.001);
}
void FRealTimePCGFoliageEdMode::BindCommands()
{
	const FPCGFoliageEditorCommands& Commands = FPCGFoliageEditorCommands::Get();
	UICommandList->MapAction(
		Commands.SetPaintBiome,
		FExecuteAction::CreateRaw(this, &FRealTimePCGFoliageEdMode::SetPaintBiome),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(this, &FRealTimePCGFoliageEdMode::GetPaintBiome)
	);
	UICommandList->MapAction(
		Commands.SetPaintSpecies,
		FExecuteAction::CreateRaw(this, &FRealTimePCGFoliageEdMode::SetPaintSpecies),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(this,&FRealTimePCGFoliageEdMode::GetPaintSpecies)
	);
}

void FRealTimePCGFoliageEdMode::CopyRenderTargetToTexture(UTexture2D* InTexture, UTextureRenderTarget2D* InRenderTarget)
{
	InRenderTarget->UpdateTexture2D(InTexture,ETextureSourceFormat::TSF_G8);
	InTexture->PostEditChange();
}

void FRealTimePCGFoliageEdMode::CleanProcedualFoliageInstance(UWorld* InWorld,FGuid Guid,const UFoliageType* FoliageType,FVector4 DirtyRect)
{

	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(InWorld->GetCurrentLevel());
	if (!IFA)
		return;
	FFoliageInfo* Info =  IFA->FindInfo(FoliageType);
	if (Info)
	{
		TArray<int32> InstancesToRemove;
		for (int32 InstanceIdx = 0; InstanceIdx < Info->Instances.Num(); InstanceIdx++)
		{
			FFoliageInstance Instance = Info->Instances[InstanceIdx];
			if (Instance.ProceduralGuid == Guid)
			{
				if (Instance.Location.X < DirtyRect.X || Instance.Location.X >= DirtyRect.Z || Instance.Location.Y < DirtyRect.Y || Instance.Location.Y >= DirtyRect.W)
				{
					continue;
				}
				InstancesToRemove.Add(InstanceIdx);
			}
		}

		if (InstancesToRemove.Num())
		{
			Info->RemoveInstances(IFA, InstancesToRemove, true);
		}
	}
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
		AddInstancesImp(InWorld, FoliageType, Instances, TArray<int32>(), nullptr, &OverrideGeometryFilter, InRebuildFoliageTree);
	}
}

void FRealTimePCGFoliageEdMode::SpawnFoliageInstance(UWorld* InWorld, const UFoliageType* Settings, const TArray<FFoliageInstance>& PlacedInstances)
{

	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(InWorld, true);
	UFoliageType* FoliageType = const_cast<UFoliageType*>(Settings);
	FFoliageInfo* FoliageInfo = IFA->FindOrAddMesh(FoliageType);

	TArray<const FFoliageInstance*> PointerArray;

	
	for (const FFoliageInstance& Instance : PlacedInstances)
	{
		PointerArray.Add(&Instance);
	}
	FoliageInfo->AddInstances(IFA, FoliageType, PointerArray);
	if (FoliageInfo->GetComponent())
		FoliageInfo->GetComponent()->BuildTreeIfOutdated(true, true);

	IFA->RegisterAllComponents();


}

bool FRealTimePCGFoliageEdMode::AddInstancesImp(UWorld* InWorld, const UFoliageType* Settings, const TArray<FDesiredFoliageInstance>& DesiredInstances, const TArray<int32>& ExistingInstances, const FPCGFoliageUISettings* UISettings, const FFoliagePaintingGeometryFilter* OverrideGeometryFilter, bool InRebuildFoliageTree)
{

	if (DesiredInstances.Num() == 0)
	{
		return false;
	}

	TArray<FPotentialInstance> PotentialInstanceBuckets;

		//@TODO: actual threaded part coming, need parts of this refactor sooner for content team
	CalculatePotentialInstances_ThreadSafe(InWorld, Settings, &DesiredInstances, PotentialInstanceBuckets, nullptr, OverrideGeometryFilter);

	// Existing foliage types in the palette  we want to override any existing mesh settings with the procedural settings.
	TMap<AInstancedFoliageActor*, TArray<const UFoliageType*>> UpdatedTypesByIFA;
	for (auto& PotentialInst : PotentialInstanceBuckets)
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


	bool bPlacedInstances = false;
	int AdditionalInstances = DesiredInstances.Num();
	TArray<FFoliageInstance> PlacedInstances;
	PlacedInstances.Reserve(AdditionalInstances);
	for (FPotentialInstance& PotentialInstance : PotentialInstanceBuckets)
	{				
		FFoliageInstance Inst;
		if (PotentialInstance.PlaceInstance(InWorld, Settings, Inst))
		{
			Inst.ProceduralGuid = PotentialInstance.DesiredInstance.ProceduralGuid;
			Inst.BaseComponent = PotentialInstance.HitComponent;
			//Inst.
			PlacedInstances.Add(MoveTemp(Inst));
			bPlacedInstances = true;
		}
		
	}
	SpawnFoliageInstance(InWorld, Settings, PlacedInstances);

	return bPlacedInstances;
}

bool FRealTimePCGFoliageEdMode::AddPotentialInstances(UWorld* InWorld,const TArray<FPotentialInstance>& PotentialInstances)
{
	TMap<const UFoliageType*, TArray<FPotentialInstance>> SettingsInstancesMap;
	for (const FPotentialInstance& DesiredInst : PotentialInstances)
	{
		TArray<FPotentialInstance>& Instances = SettingsInstancesMap.FindOrAdd(DesiredInst.DesiredInstance.FoliageType);
		Instances.Add(DesiredInst);
	}
	for (auto It = SettingsInstancesMap.CreateConstIterator(); It; ++It)
	{
		const UFoliageType* FoliageType = It.Key();

		const TArray<FPotentialInstance>& Instances = It.Value();
		AddPotentialInstancesImp(InWorld, FoliageType, Instances);
	}
	return true;
}

bool FRealTimePCGFoliageEdMode::AddPotentialInstancesImp(UWorld* InWorld, const UFoliageType* Settings,const TArray<FPotentialInstance>& PotentialInstances)
{
	bool bPlacedInstances = false;
	int AdditionalInstances = PotentialInstances.Num();
	TArray<FFoliageInstance> PlacedInstances;
	PlacedInstances.Reserve(AdditionalInstances);
	for (const FPotentialInstance& PotentialInstance : PotentialInstances)
	{
		FFoliageInstance Inst;
		if (const_cast<FPotentialInstance&>(PotentialInstance).PlaceInstance(InWorld, Settings, Inst))
		{
			Inst.ProceduralGuid = PotentialInstance.DesiredInstance.ProceduralGuid;
			Inst.BaseComponent = PotentialInstance.HitComponent;
			//Inst.
			PlacedInstances.Add(MoveTemp(Inst));
			bPlacedInstances = true;
		}

	}
	SpawnFoliageInstance(InWorld, Settings, PlacedInstances);
	return true;
}

void FRealTimePCGFoliageEdMode::CalculatePotentialInstances_ThreadSafe(const UWorld* InWorld, const UFoliageType* Settings, const TArray<FDesiredFoliageInstance>* DesiredInstances, TArray<FPotentialInstance>& OutPotentialInstances, const FPCGFoliageUISettings* UISettings, const FFoliagePaintingGeometryFilter* OverrideGeometryFilter)
{
	double start = FPlatformTime::Seconds();

	for (int32 InstanceIdx = 0; InstanceIdx < DesiredInstances->Num(); ++InstanceIdx)
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
			OutPotentialInstances.Add(FPotentialInstance(Hit.ImpactPoint, Hit.ImpactNormal, Hit.Component.Get(), HitWeight, DesiredInst));

		}
	}

	double end1 = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Warning, TEXT("TraceAll executed in %f seconds."), end1 - start);
}

APCGFoliageManager* FRealTimePCGFoliageEdMode::GetPCGFoliageManager(bool bCreateIfNone)
{
	if (!PCGFoliageManager.IsValid())
	{
		APCGFoliageManager* Result;
		for (TActorIterator<APCGFoliageManager> ActorItr(GetWorld()); ActorItr; ++ActorItr)
		{
			Result = Cast<APCGFoliageManager>(*ActorItr);
			if (Result)
			{
				PCGFoliageManager = Result;
				break;				
			}
		}
	}
	return PCGFoliageManager.Get();

}

UBiome* FRealTimePCGFoliageEdMode::GetEditedBiome()
{
	return ((FRealTimePCGFoliageEdModeToolkit*)Toolkit.Get())->GetSelectBiome();
}

USpecies* FRealTimePCGFoliageEdMode::GetEditedSpecies()
{
	return ((FRealTimePCGFoliageEdModeToolkit*)Toolkit.Get())->GetSelectSpecies();
}

UTexture2D* FRealTimePCGFoliageEdMode::GetEditedSpeciesCleanTexture()
{
	USpecies* EditedSpecies = GetEditedSpecies();
	if (!EditedSpecies)
		return nullptr;
	UBiome* Biome = GetEditedBiome();
	if (!Biome)
		return nullptr;
	int32 Index = Biome->Species.IndexOfByKey(EditedSpecies);
	return GetEditedBiomeData()->CleanMaps[Index];
}

FBiomeData* FRealTimePCGFoliageEdMode::GetEditedBiomeData()
{
	UBiome* EditedBiome=GetEditedBiome();
	if (!EditedBiome)
		return nullptr;
	int32 Index = GetPCGFoliageManager()->Biomes.IndexOfByKey(EditedBiome);
	return &GetPCGFoliageManager()->BiomeData[Index];
}

void FPCGFoliageUISettings::Load()
{
	GConfig->GetFloat(TEXT("PCGFoliageEdit"), TEXT("Radius"), Radius, GEditorPerProjectIni);
	GConfig->GetFloat(TEXT("PCGFoliageEdit"), TEXT("FallOff"), FallOff, GEditorPerProjectIni);

	GConfig->GetBool(TEXT("PCGFoliageEdit"), TEXT("bPaintBiome"), bPaintBiome, GEditorPerProjectIni);
	GConfig->GetBool(TEXT("PCGFoliageEdit"), TEXT("bPaintSpecies"), bPaintSpecies, GEditorPerProjectIni);

	IsInQuickEraseMode = false;
}

void FPCGFoliageUISettings::Save()
{
	GConfig->SetFloat(TEXT("PCGFoliageEdit"), TEXT("Radius"), Radius, GEditorPerProjectIni);
	GConfig->SetFloat(TEXT("PCGFoliageEdit"), TEXT("FallOff"), FallOff, GEditorPerProjectIni);

	GConfig->SetBool(TEXT("PCGFoliageEdit"), TEXT("bPaintBiome"), bPaintBiome, GEditorPerProjectIni);
	GConfig->SetBool(TEXT("PCGFoliageEdit"), TEXT("bPaintSpecies"), bPaintSpecies, GEditorPerProjectIni);
}
