// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealTimePCGFoliageEdModeToolkit.h"
#include "RealTimePCGFoliageEdMode.h"
#include "Engine/Selection.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorModeManager.h"
#include "SPCGFoliageEdit.h"
#include "SPCGFoliagePalette.h"
#define LOCTEXT_NAMESPACE "FRealTimePCGFoliageEdModeToolkit"

namespace
{
	static const FName PCGFoliageName(TEXT("PCGFoliage"));
	const TArray<FName> PCGFoliagePaletteNames = { PCGFoliageName };
}

FRealTimePCGFoliageEdModeToolkit::FRealTimePCGFoliageEdModeToolkit()
{
}

void FRealTimePCGFoliageEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	PCGFoliageEdWidget = SNew(SPCGFoliageEdit);
	
	FModeToolkit::Init(InitToolkitHost);
}

FName FRealTimePCGFoliageEdModeToolkit::GetToolkitFName() const
{
	return FName("RealTimePCGFoliageEdMode");
}

FText FRealTimePCGFoliageEdModeToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("RealTimePCGFoliageEdModeToolkit", "DisplayName", "RealTimePCGFoliageEdMode Tool");
}

class FEdMode* FRealTimePCGFoliageEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FRealTimePCGFoliageEdMode::EM_RealTimePCGFoliageEdModeId);
}

TSharedPtr<class SWidget> FRealTimePCGFoliageEdModeToolkit::GetInlineContent() const
{
	return PCGFoliageEdWidget;
}

void FRealTimePCGFoliageEdModeToolkit::GetToolPaletteNames(TArray<FName>& InPaletteName) const
{
	InPaletteName = PCGFoliagePaletteNames;
}

FText FRealTimePCGFoliageEdModeToolkit::GetToolPaletteDisplayName(FName PaletteName) const
{
	if (PaletteName == PCGFoliageName)
	{
		return LOCTEXT("PCGFoliage", "PCGFoliage");
	}
	return FText();
}

void FRealTimePCGFoliageEdModeToolkit::BuildToolPalette(FName PaletteName, FToolBarBuilder& ToolbarBuilder)
{
	const FPCGFoliageEditorCommands& Commands = FPCGFoliageEditorCommands::Get();
	ToolbarBuilder.AddToolBarButton(Commands.SetPaintBiome);
	ToolbarBuilder.AddToolBarButton(Commands.SetPaintSpecies);

	if (PaletteName == PCGFoliageName)
	{
		PCGFoliageEdWidget->CustomizeToolBarPalette(ToolbarBuilder);
	}
}

void FRealTimePCGFoliageEdModeToolkit::CleanSlate()
{
	PCGFoliageEdWidget = nullptr;
}

UBiome* FRealTimePCGFoliageEdModeToolkit::GetSelectBiome()
{
	TArray<TSharedPtr<FBiomeModel>> Result = PCGFoliageEdWidget->Palette->BiomeListView->GetSelectedItems();
	if (Result.Num() == 1)
	{
		return Result[0]->Biome.Get();
	}
	else if (Result.Num() >= 2)
	{
		return nullptr;
	}
	else
	{
		return nullptr;
	};
	//return EdMode->;
}

USpecies* FRealTimePCGFoliageEdModeToolkit::GetSelectSpecies()
{
	TArray<TSharedPtr<FSpeceiesModel>> Result = PCGFoliageEdWidget->Palette->SpeceiesListView->GetSelectedItems();
	if (Result.Num() == 1)
	{
		return Result[0]->Species.Get();
	}
	else if(Result.Num()>=2)
	{
		return nullptr;
	}
	else
	{
		return nullptr;
	}
	
}

#undef LOCTEXT_NAMESPACE
