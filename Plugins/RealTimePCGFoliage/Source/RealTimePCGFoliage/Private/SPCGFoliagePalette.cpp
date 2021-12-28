// Copyright Epic Games, Inc. All Rights Reserved.

#include "SPCGFoliagePalette.h"


#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Slate/DeferredCleanupSlateBrush.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"


FSpeceiesModel::FSpeceiesModel(USpecies* InSpecies, UTexture2D* InTexture):Species(InSpecies), Texture(InTexture)
{
	if(Texture!=nullptr)
		SlateBrush = FDeferredCleanupSlateBrush::CreateBrush(Texture.Get(),FVector2D(64,64));	
}

TSharedRef<SWidget> FSpeceiesModel::CreateWidget(TSharedPtr<FAssetThumbnailPool> InThumbnailPool) const
{
	return SNew(STextBlock)
		.Text(FText::FromString(Species->GetName()));
}

TSharedRef<SWidget> FSpeceiesModel::CreateCleanRTPreview() const
{
	const FSlateBrush* SlateBrushPtr;
	SlateBrushPtr = FDeferredCleanupSlateBrush::TrySlateBrush(SlateBrush);
	if (!SlateBrushPtr)
		SlateBrushPtr=FEditorStyle::GetDefaultBrush();

	return SNew(SImage)
		.Image(SlateBrushPtr);
}

TSharedRef<ITableRow> SPCGFoliagePalette::GenerateRowForSpeciesList(TSharedPtr<FSpeceiesModel> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FString A = Item->Species.IsValid() ? Item->Species->GetName() : FString("nullptr");
	FString B = Item->Texture.IsValid() ? Item->Texture->GetName() : FString("nullptr");
	return	
		SNew(STableRow<TSharedPtr<FSpeceiesModel>>, OwnerTable)
		.Padding(2.0f)		
		[			
			SNew(SHorizontalBox)			
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				Item->CreateWidget(ThumbnailPool)
				
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				Item->CreateCleanRTPreview()
			]			
		];
}
TSharedRef<ITableRow> SPCGFoliagePalette::GenerateRowForBiomeList(TSharedPtr<FBiomeModel> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FSpeceiesModel>>, OwnerTable)
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(4.0f)
			.AutoWidth()
			.HAlign(EHorizontalAlignment::HAlign_Left)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->Biome->GetName()))
				.ColorAndOpacity(FLinearColor::White)			

			]
			+ SHorizontalBox::Slot()
			.Padding(4.0f)
			.HAlign(EHorizontalAlignment::HAlign_Right)				
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(4.0f)
				.AutoWidth()
				.HAlign(EHorizontalAlignment::HAlign_Right)
				[
					SNew(SColorBlock)
					.Color(Item->PreviewColor)
					
				]

			]
		];;
}
void SPCGFoliagePalette::RefreshBiomeModels()
{
	BiomeModels.Empty();
	TArray<UBiome*>& Biomes = EdMode->GetPCGFoliageManager()->Biomes;
	TArray<FLinearColor> PreviewColor = EdMode->GetPCGFoliageManager()->GetBiomePreviewColor();

	int Num = Biomes.Num();
	for (int i = 0; i < Num; i++)
	{
		UBiome* BiomePtr =Biomes[i] ;

		TSharedPtr<FBiomeModel> Model = MakeShareable(new FBiomeModel(BiomePtr, PreviewColor[i]));
		///*if (i < CleanRTs.Num())
		//	Model->Texture = CleanRTs[i];*/
		//Model->Texture = EdMode->GetEditedRT();
		BiomeModels.Add(Model);
	}
}
void SPCGFoliagePalette::RefreshSpeceiesModels()
{
	SpeceiesModels.Empty();
	UBiome* EditedBiome;
	EditedBiome = EdMode->GetEditedBiome();
	if (!EditedBiome)
		return;

	TArray<USpecies*>& Species = EditedBiome->Species;
	TArray<UTexture2D*>& CleanRTs = EdMode->GetEditedBiomeData()->CleanMaps;
	int Num = FMath::Max(CleanRTs.Num(), Species.Num());
	for (int i = 0; i < Num; i++)
	{
		USpecies* SpeciesPtr= i < Species.Num()? Species[i]:nullptr;

		TSharedPtr<FSpeceiesModel> Model = MakeShareable(new FSpeceiesModel(SpeciesPtr, CleanRTs[i]));
		SpeceiesModels.Add(Model);
	}
	SpeceiesListView->RequestListRefresh();
}
void SPCGFoliagePalette::OnSelectedBiomeChange(TSharedPtr<FBiomeModel> BiomeModel, ESelectInfo::Type SelectType)
{
	RefreshSpeceiesModels();
}
////////////////////////////////////////////////
// SPCGFoliagePalette
////////////////////////////////////////////////
void SPCGFoliagePalette::Construct(const FArguments& InArgs)
{
	const FText BlankText = FText::GetEmpty();
	EdMode = (FRealTimePCGFoliageEdMode*)GLevelEditorModeTools().GetActiveMode(FRealTimePCGFoliageEdMode::EM_RealTimePCGFoliageEdModeId);
	BiomePreviewSlateBrush = FDeferredCleanupSlateBrush::CreateBrush(EdMode->BiomePreviewRenderTarget, FVector2D(256, 256));
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(64));
	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Vertical)
		.Style(FEditorStyle::Get(), "FoliageEditMode.Splitter")
		+ SSplitter::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(BiomeListView, SListView<TSharedPtr<FBiomeModel>>)
				.OnGenerateRow(this, &SPCGFoliagePalette::GenerateRowForBiomeList)
				.ItemHeight(32)
				.ListItemsSource(&BiomeModels)
				.SelectionMode(ESelectionMode::Single)
				.OnSelectionChanged(this, &SPCGFoliagePalette::OnSelectedBiomeChange)
			]
			/*+ SVerticalBox::Slot()
			.MaxHeight(128)				
			.HAlign(EHorizontalAlignment::HAlign_Center)
				
			[
				SNew(SBox)
				.WidthOverride(128)
				.HeightOverride(128)
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.MaxWidth(128)
				.VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(SImage)
					.Image(FDeferredCleanupSlateBrush::TrySlateBrush(BiomePreviewSlateBrush))
				]
			]*/
			
		]
		+SSplitter::Slot()
		[
			SNew(SBox)
			.WidthOverride(128)
			.HeightOverride(128)
			.MaxAspectRatio(1)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.VAlign(EVerticalAlignment::VAlign_Center)
			[
				SNew(SImage)
				.Image(FDeferredCleanupSlateBrush::TrySlateBrush(BiomePreviewSlateBrush))
				.RenderOpacity(1)
			]
		]
		// Details
		+ SSplitter::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SAssignNew(SpeceiesListView, SListView<TSharedPtr<FSpeceiesModel>>)
				.OnGenerateRow(this, &SPCGFoliagePalette::GenerateRowForSpeciesList)
				.ItemHeight(64)
				.ListItemsSource(&SpeceiesModels)
				.IsEnabled(EdMode,&FRealTimePCGFoliageEdMode::GetPaintSpecies)
			]
		]
	];
	RefreshBiomeModels();
	SpeceiesModels.Empty();
}



#undef LOCTEXT_NAMESPACE

FBiomeModel::FBiomeModel(UBiome* InBiome, FLinearColor InPreviewColor):Biome(InBiome),PreviewColor(InPreviewColor)
{

}
