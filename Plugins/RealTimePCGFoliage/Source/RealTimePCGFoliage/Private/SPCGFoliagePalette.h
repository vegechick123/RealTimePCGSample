// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "RealTimePCGFoliageEdMode.h"
#include "Slate/DeferredCleanupSlateBrush.h"
struct FSpeceiesModel
{
	TWeakObjectPtr<USpecies> Species;
	TWeakObjectPtr<UTexture2D> Texture;
	TSharedPtr<FDeferredCleanupSlateBrush> SlateBrush;
	FSpeceiesModel(USpecies* InSpecies, UTexture2D* InTexture);
	TSharedRef<SWidget> CreateWidget(TSharedPtr<FAssetThumbnailPool> InThumbnailPool)const;
	TSharedRef<SWidget> CreateCleanRTPreview()const;
};
struct FBiomeModel
{
	TWeakObjectPtr<UBiome> Biome;
	FColor PreviewColor;
	FBiomeModel(UBiome* InBiome, FColor InPreviewColor);
};
/** The palette of foliage types available for use by the foliage edit mode */
class SPCGFoliagePalette : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPCGFoliagePalette) {}
		SLATE_ARGUMENT(SPCGFoliagePalette*, FoliageEdMode)
	SLATE_END_ARGS()

		TSharedRef<ITableRow> GenerateRowForSpeciesList(TSharedPtr<FSpeceiesModel> Item, const TSharedRef<STableViewBase>& OwnerTable);
		TSharedRef<ITableRow> GenerateRowForBiomeList(TSharedPtr<FBiomeModel> Item, const TSharedRef<STableViewBase>& OwnerTable);
		void RefreshBiomeModels();
		void RefreshSpeceiesModels();
		void OnSelectedBiomeChange(TSharedPtr<FBiomeModel> BiomeModel,ESelectInfo::Type SelectType);
		TArray<TSharedPtr<FSpeceiesModel>> SpeceiesModels;
		TArray<TSharedPtr<FBiomeModel>> BiomeModels;
		TSharedPtr<SListView<TSharedPtr<FSpeceiesModel>>> SpeceiesListView;
		TSharedPtr<SListView<TSharedPtr<FBiomeModel>>> BiomeListView;
		TSharedPtr<class FAssetThumbnailPool> ThumbnailPool;
		FRealTimePCGFoliageEdMode* EdMode;
		void Construct(const FArguments& InArgs);

	
};
