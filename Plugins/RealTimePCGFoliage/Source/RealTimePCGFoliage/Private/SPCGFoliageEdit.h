// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 
#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

class FRealTimePCGFoliageEdMode;
class SPCGFoliagePalette;

class SPCGFoliageEdit : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPCGFoliageEdit) {}
	SLATE_END_ARGS()

public:
	/** SCompoundWidget functions */
	void Construct(const FArguments& InArgs);

	
	/** Get the error message for this editing mode */
	//FText GetFoliageEditorErrorText() const;

	/** Modes Panel Header Information **/
	void CustomizeToolBarPalette(FToolBarBuilder& ToolBarBuilder);
	//FText GetActiveToolName() const;
	//FText GetActiveToolMessage() const;


private:
	/** Creates the toolbar. */
	//TSharedRef<SWidget> BuildToolBar();

	/** Checks if the tool mode is Paint. */

public:
	/** Palette of available foliage types */
	TSharedPtr<SPCGFoliagePalette> Palette;
	
	/** Current error message */	
	TSharedPtr<class SErrorText> ErrorText;
		
	/** Pointer to the foliage edit mode. */
	FRealTimePCGFoliageEdMode*	EdMode;
};
