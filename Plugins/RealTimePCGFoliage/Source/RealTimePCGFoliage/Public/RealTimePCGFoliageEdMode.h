// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdMode.h"
#include "PCGFoliageEditorCommands.h"
#include "PCGFoliageManager.h"

// Current user settings in Foliage UI
struct FFoliageUISettings
{
	void Load();
	void Save();

	// tool
	bool GetPaintToolSelected() const { return true; }
	void SetPaintToolSelected(bool InbPaintToolSelected) { bPaintToolSelected = InbPaintToolSelected; }


	float GetRadius() const { return Radius; }
	void SetRadius(float InRadius) {  Radius = InRadius; }
	float GetPaintDensity() const { return PaintDensity; }
	void SetPaintDensity(float InPaintDensity) { PaintDensity = InPaintDensity; }

	bool bPaintToolSelected;
	bool bSpeciesEraseSelected;

	float Radius;
	float PaintDensity;
	
};
struct FFoliagePaintingGeometryFilter
{
	bool bAllowLandscape;
	bool bAllowStaticMesh;
	bool bAllowBSP;
	bool bAllowFoliage;
	bool bAllowTranslucent;
	FFoliagePaintingGeometryFilter()
		: bAllowLandscape(true)
		, bAllowStaticMesh(false)
		, bAllowBSP(false)
		, bAllowFoliage(false)
		, bAllowTranslucent(false)
	{
	}

	bool operator() (const UPrimitiveComponent* Component) const;
};


class FRealTimePCGFoliageEdMode : public FEdMode
{
public:
	const static FEditorModeID EM_RealTimePCGFoliageEdModeId;
public:
	FRealTimePCGFoliageEdMode();
	virtual ~FRealTimePCGFoliageEdMode();
	

	FFoliageUISettings UISettings;

	/** Command list lives here so that the key bindings on the commands can be processed in the viewport. */
	TSharedPtr<FUICommandList> UICommandList;

	TWeakObjectPtr<APCGFoliageManager> PCGFoliageManager;

	class UMaterialInstanceDynamic* PaintMID;
	UTextureRenderTarget2D* PaintRTCache;
	bool bBrushTraceValid;
	FVector BrushLocation;
	FVector BrushNormal;
	FVector BrushTraceDirection;
	UStaticMeshComponent* SphereBrushComponent;

	/** The dynamic material of the sphere brush. */
	class UMaterialInstanceDynamic* BrushMID;
	FColor BrushDefaultHighlightColor;
	FColor BrushCurrentHighlightColor;

	/** Default opacity received from the brush material to reset it when closing. */
	float DefaultBrushOpacity;
	bool bToolActive;
	bool bCanAltDrag;
	bool bAdjustBrushRadius;

	/** Flag to know when we are tracking a transaction in mouse delta */
	bool bTracking;

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	// FEdMode interface
	virtual void Enter() override;
	virtual void Exit() override;

	/** FEdMode: Called after an Undo operation */
	virtual void PostUndo() override;

	//virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	//virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	//virtual void ActorSelectionChangeNotify() override;
	bool UsesToolkits() const override;

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
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;

	/**
	* FEdMode: Called when the mouse is moved while a window input capture is in effect
	*
	* @param	InViewportClient	Level editor viewport client that captured the mouse input
	* @param	InViewport			Viewport that captured the mouse input
	* @param	InMouseX			New mouse cursor X coordinate
	* @param	InMouseY			New mouse cursor Y coordinate
	*
	* @return	true if input was handled
	*/
	virtual bool CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;

	/** FEdMode: Called when a mouse button is pressed */
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	
	/** Ends tracking and end potential transaction */
	bool EndTracking();

	/** FEdMode: Called when a mouse button is released */
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;

	/** FEdMode: Called once per frame */
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;

	/** FEdMode: Called when a key is pressed */
	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;

	/** FEdMode: Called when mouse drag input it applied */
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;

	/** FEdMode: Render elements for the Foliage tool */
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;

	/** FEdMode: Render HUD elements for this tool */
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;

	/** FEdMode: Check to see if an actor can be selected in this mode - no side effects */
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelection) const override;
	/** Notifies all active modes of mouse click messages. */
	bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	// End of FEdMode interface

	/** Start foliage tracing */
	void StartFoliageBrushTrace(FEditorViewportClient* ViewportClient, class UViewportInteractor* Interactor = nullptr);

	/* End foliage tracing*/
	void EndFoliageBrushTrace();

	/** Trace under the mouse cursor and update brush position */
	void FoliageBrushTrace(FEditorViewportClient* ViewportClient, const FVector& InRayOrigin, const FVector& InRayDirection);

	/** Setup before call to ApplyBrush */
	void PreApplyBrush();

	/** Apply brush */
	void ApplyBrush(FEditorViewportClient* ViewportClient);
	
	/** Adjusts the radius of the foliage brush, using the given multiplier to adjust speed */
	void AdjustBrushRadius(float Multiplier);
	
	/** Set the brush mesh opacity */
	void SetBrushOpacity(const float InOpacity);

	void SetSpeciesErase();

	bool GetSpeciesErase()const;
	float GetPaintingBrushRadius() const;

	ALandscape* GetLandscape() const;

	UTexture2D* GetEditedTexture();

	void SetPaintMaterial();
	
	void BindCommands();
	static void CopyRenderTargetToTexture(UTexture2D* InTexture,UTextureRenderTarget2D* InRenderTarget);
	
	static void CleanProcedualFoliageInstance(UWorld* InWorld, FGuid Guid,const UFoliageType* FoliageType);

	static void AddInstances(UWorld* InWorld, const TArray<FDesiredFoliageInstance>& DesiredInstances, const FFoliagePaintingGeometryFilter& OverrideGeometryFilter = FFoliagePaintingGeometryFilter(), bool InRebuildFoliageTree = true);

	/** Common code for adding instances to world based on settings */
	static bool AddInstancesImp(UWorld* InWorld,const UFoliageType* Settings, const TArray<FDesiredFoliageInstance>& DesiredInstances, const TArray<int32>& ExistingInstances = TArray<int32>(), const FFoliageUISettings* UISettings = nullptr, const FFoliagePaintingGeometryFilter* OverrideGeometryFilter = nullptr, bool InRebuildFoliageTree = true);

	/** Similar to CalculatePotentialInstances, but it doesn't do any overlap checks which are much harder to thread. Meant to be run in parallel for placing lots of instances */
	static void CalculatePotentialInstances_ThreadSafe(const UWorld* InWorld, const UFoliageType* Settings, const TArray<FDesiredFoliageInstance>* DesiredInstances, TArray<FPotentialInstance>& OutPotentialInstances, const FFoliageUISettings* UISettings, const FFoliagePaintingGeometryFilter* OverrideGeometryFilter);

	APCGFoliageManager* GetPCGFoliageManager(bool bCreateIfNone = false);

	UBiome* GetEditedBiome();
	USpecies* GetEditedSpecies();
	UTexture2D* GetEditedSpeciesCleanTexture();
	FBiomeRenderTargetData* GetEditedBiomeData();

};
