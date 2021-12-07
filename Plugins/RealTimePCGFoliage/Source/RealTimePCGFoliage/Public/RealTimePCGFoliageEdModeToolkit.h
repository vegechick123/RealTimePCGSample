// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Biome.h"
#include "Toolkits/BaseToolkit.h"

class SFoliageEdit;
class FRealTimePCGFoliageEdModeToolkit : public FModeToolkit
{
public:

	FRealTimePCGFoliageEdModeToolkit();
	
	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual class FEdMode* GetEditorMode() const override;
	virtual TSharedPtr<class SWidget> GetInlineContent() const override;
	//virtual TSharedPtr<class SWidget> GetInlineContent() const override { return ToolkitWidget; }
	
	/** Mode Toolbar Palettes **/
	virtual void GetToolPaletteNames(TArray<FName>& InPaletteName) const;
	virtual FText GetToolPaletteDisplayName(FName PaletteName) const;
	virtual void BuildToolPalette(FName PaletteName, class FToolBarBuilder& ToolbarBuilder);
	UBiome* GetSelectBiome();
	USpecies* GetSelectSpecies();

private:

	TSharedPtr<class SPCGFoliageEdit> PCGFoliageEdWidget;
};
