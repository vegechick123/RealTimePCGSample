// Copyright Epic Games, Inc. All Rights Reserved.

#include "SPCGFoliageEdit.h"
#include "RealTimePCGFoliageEdMode.h"
#include "SPCGFoliagePalette.h"
#include "EditorModeManager.h"
#include "Editor/PropertyEditor/Public/VariablePrecisionNumericInterface.h"
#include "Widgets/Input/SSpinBox.h"

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
	float UIMin = 16;
	float UIMax = 8192;
	float SliderExponent = 1.0f;
	TSharedPtr<INumericTypeInterface<float>> NumericInterface = MakeShareable(new FVariablePrecisionNumericInterface());

	TSharedRef<SWidget> RadiusControl = SNew(SSpinBox<float>)
		.Style(&FEditorStyle::Get().GetWidgetStyle<FSpinBoxStyle>("LandscapeEditor.SpinBox"))
		.PreventThrottling(true)
		.MinValue(UIMin)
		.MaxValue(UIMax)
		.SliderExponent(2)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
		.MinDesiredWidth(40.f)
		.TypeInterface(NumericInterface)
		.Justification(ETextJustify::Center)
		.ContentPadding(FMargin(0.f, 2.f, 0.f, 0.f))
		.Value_Lambda([this]() -> float { return EdMode->UISettings.Radius;})
		.OnValueChanged_Lambda([this](float NewValue) { EdMode->UISettings.Radius = NewValue; });
	ToolBarBuilder.AddToolBarWidget(RadiusControl, LOCTEXT("BrushRadius", "Radius"));

	TSharedRef<SWidget> FallOffControl = SNew(SSpinBox<float>)
		.Style(&FEditorStyle::Get().GetWidgetStyle<FSpinBoxStyle>("LandscapeEditor.SpinBox"))
		.PreventThrottling(true)
		.MinValue(0.0001)
		.MaxValue(1)
		.SliderExponent(1)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
		.MinDesiredWidth(40.f)
		.TypeInterface(NumericInterface)
		.Justification(ETextJustify::Center)
		.ContentPadding(FMargin(0.f, 2.f, 0.f, 0.f))
		.Value_Lambda([this]() -> float { return EdMode->UISettings.FallOff; })
		.OnValueChanged_Lambda([this](float NewValue) { EdMode->UISettings.FallOff = NewValue; });
	ToolBarBuilder.AddToolBarWidget(FallOffControl, LOCTEXT("BrushFallOff", "FallOff"));

	ToolBarBuilder.AddToolBarButton(
		FUIAction(nullptr),
		NAME_None,
		LOCTEXT("FoliageSelectAll", "All"),
		LOCTEXT("FoliageSelectAllTooltip", "Select All Foliage"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "FoliageEditMode.SelectAll")
	);

}
#undef LOCTEXT_NAMESPACE
