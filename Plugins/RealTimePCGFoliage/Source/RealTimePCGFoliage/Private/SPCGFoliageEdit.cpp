// Copyright Epic Games, Inc. All Rights Reserved.

#include "SPCGFoliageEdit.h"
#include "RealTimePCGFoliageEdMode.h"
#include "SPCGFoliagePalette.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "RealTimePCGFoliageEd_Mode"

void SPCGFoliageEdit::Construct(const FArguments& InArgs)
{
	EdMode = (FRealTimePCGFoliageEdMode*)GLevelEditorModeTools().GetActiveMode(FRealTimePCGFoliageEdMode::EM_RealTimePCGFoliageEdModeId);
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.VAlign(VAlign_Fill)
		.Padding(0.f, 5.f, 0.f, 0.f)
		[
			SAssignNew(Palette, SPCGFoliagePalette)
		]
	];
}
void SPCGFoliageEdit::CustomizeToolBarPalette(FToolBarBuilder& ToolBarBuilder)
{
	// NOTES 
	// 
	// Legacy Foliage mode treated single instance as a setting rather than a tool.
	// The following code can be cleaned up once the Legacy Foliage Mode is removed, by clearing
	// the SingleInstantiation flag in FEdModeFoliage::ClearAllToolSelection.  Instead here,
	// we explicitly clear it in Paint, Reapply, and Fill 
	// 

	//  Select All
	ToolBarBuilder.AddToolBarButton(
		FUIAction(nullptr),
		NAME_None,
		LOCTEXT("FoliageSelectAll", "All"),
		LOCTEXT("FoliageSelectAllTooltip", "Select All Foliage"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "FoliageEditMode.SelectAll")
	);

}
#undef LOCTEXT_NAMESPACE
