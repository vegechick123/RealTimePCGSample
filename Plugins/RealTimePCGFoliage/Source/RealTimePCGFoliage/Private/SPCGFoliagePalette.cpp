// Copyright Epic Games, Inc. All Rights Reserved.

#include "SPCGFoliagePalette.h"


#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSearchBox.h"
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
	;
	//FAssetData AssetData = FAssetData(Species->FoliageTypes[0]);
	//TSharedPtr< FAssetThumbnail > Thumbnail = MakeShareable(new FAssetThumbnail(Species->FoliageTypes[0], 8, 8, InThumbnailPool));

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
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->Biome->GetName()))

			]
		];;
}
void SPCGFoliagePalette::RefreshBiomeModels()
{
	BiomeModels.Empty();
	TArray<UBiome*>& Biomes = EdMode->GetPCGFoliageManager()->Biomes;
	TArray<FColor> Colors;
	Colors.Add(FColor::Red);
	Colors.Add(FColor::Green);
	Colors.Add(FColor::Blue);
	int Num = Biomes.Num();
	for (int i = 0; i < Num; i++)
	{
		UBiome* BiomePtr =Biomes[i] ;

		TSharedPtr<FBiomeModel> Model = MakeShareable(new FBiomeModel(BiomePtr, Colors[i]));
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
	//SpeceiesModels.Add(MakeShareable(new FSpeceiesModel(nullptr, nullptr)));
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
	
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(64));
	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Vertical)
		.Style(FEditorStyle::Get(), "FoliageEditMode.Splitter")
		+ SSplitter::Slot()
		[
			SAssignNew(BiomeListView, SListView<TSharedPtr<FBiomeModel>>)
			.OnGenerateRow(this, &SPCGFoliagePalette::GenerateRowForBiomeList)
			.ItemHeight(32)
			.ListItemsSource(&BiomeModels)
			.SelectionMode(ESelectionMode::Single)
			.OnSelectionChanged(this, &SPCGFoliagePalette::OnSelectedBiomeChange)
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

FBiomeModel::FBiomeModel(UBiome* InBiome, FColor InPreviewColor):Biome(InBiome),PreviewColor(InPreviewColor)
{

}
