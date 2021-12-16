// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCGFoliageEditorCommands.h"



#define LOCTEXT_NAMESPACE "PCGFoliageEditCommands"

void FPCGFoliageEditorCommands::RegisterCommands()
{
	UI_COMMAND( DecreaseBrushSize, "Decrease Brush Size", "Decreases the size of the foliage brush", EUserInterfaceActionType::Button, FInputChord(EKeys::LeftBracket) );
	UI_COMMAND( IncreaseBrushSize, "Increase Brush Size", "Increases the size of the foliage brush", EUserInterfaceActionType::Button, FInputChord(EKeys::RightBracket) );

	UI_COMMAND( DecreasePaintDensity, "Decrease Brush Density", "Decreases the density of the foliage brush", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::LeftBracket) );
	UI_COMMAND( IncreasePaintDensity, "Increase Brush Density", "Increases the density of the foliage brush", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::RightBracket) );

	UI_COMMAND( DecreaseUnpaintDensity, "Decrease Erase Density", "Decreases the density of the foliage eraser", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::LeftBracket));
	UI_COMMAND( IncreaseUnpaintDensity, "Increase Erase Density", "Increases the density of the foliage eraser", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::RightBracket));

	UI_COMMAND( SetNoSettings, "Hide Details", "Hide details.", EUserInterfaceActionType::ToggleButton, FInputChord() );
	UI_COMMAND( SetPaintSettings, "Show Painting settings", "Show painting settings.", EUserInterfaceActionType::ToggleButton, FInputChord() );
	UI_COMMAND( SetClusterSettings, "Show Instance settings", "Show settings for placed instances.", EUserInterfaceActionType::ToggleButton, FInputChord() );

	UI_COMMAND(SetPaintBiome, "Paint  Biome", "Paint Selected Biome.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetPaintSpecies, "Paint Species", "Paint Selected Species.", EUserInterfaceActionType::ToggleButton, FInputChord());	
}

#undef LOCTEXT_NAMESPACE
