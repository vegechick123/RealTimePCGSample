// Copyright Epic Games, Inc. All Rights Reserved.

#include "SPCGFoliagePalette.h"


#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Slate/DeferredCleanupSlateBrush.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"


FSpeceiesModel::FSpeceiesModel(USpecies* InSpecies, UTexture2D* InCleanMap, UTextureRenderTarget2D* InDensityMap):Species(InSpecies), CleanMap(InCleanMap), DensityMap(InDensityMap)
{
	// TODO: SlateBrush对纹理的强引用导致某些情况下会发生内存泄漏，应手动管理SlateBrush的生命周期
	if(InCleanMap !=nullptr)
		CleanMapSlateBrush = FDeferredCleanupSlateBrush::CreateBrush(CleanMap.Get(),FVector2D(64,64));
	if (InDensityMap != nullptr)
		DensityMapSlateBrush = FDeferredCleanupSlateBrush::CreateBrush(DensityMap.Get(), FVector2D(64, 64));
}

TSharedRef<SWidget> FSpeceiesModel::CreateWidget() const
{
	return SNew(SBox)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Species->GetName()))
		];
		
}

TSharedRef<SWidget> FSpeceiesModel::CreateCleanMapPreview() const
{
	const FSlateBrush* SlateBrushPtr;
	SlateBrushPtr = FDeferredCleanupSlateBrush::TrySlateBrush(CleanMapSlateBrush);
	if (!SlateBrushPtr)
		SlateBrushPtr=FEditorStyle::GetDefaultBrush();

	return SNew(SBox)
		.WidthOverride(70)
		.HeightOverride(70)
		.MaxAspectRatio(1)
		.HAlign(EHorizontalAlignment::HAlign_Center)
		.VAlign(EVerticalAlignment::VAlign_Center)
		[
			SNew(SImage)
			.Image(SlateBrushPtr)
		];
}

TSharedRef<SWidget> FSpeceiesModel::CreateDensityMapPreview() const
{
	const FSlateBrush* SlateBrushPtr;
	SlateBrushPtr = FDeferredCleanupSlateBrush::TrySlateBrush(DensityMapSlateBrush);
	if (!SlateBrushPtr)
		SlateBrushPtr = FEditorStyle::GetDefaultBrush();

	return SNew(SBox)
		.WidthOverride(70)
		.HeightOverride(70)
		.MaxAspectRatio(1)
		.HAlign(EHorizontalAlignment::HAlign_Center)
		.VAlign(EVerticalAlignment::VAlign_Center)
		[
			SNew(SImage)
			.Image(SlateBrushPtr)
		
		];
}

class SSpeciesDetailRow : public SMultiColumnTableRow<TSharedPtr<FSpeceiesModel>>
{
	SLATE_BEGIN_ARGS(SSpeciesDetailRow) {}
		SLATE_ARGUMENT(TSharedPtr<FSpeceiesModel>, Item)
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		Item = InArgs._Item;
		FSuperRowType::Construct(FSuperRowType::FArguments()
			.Padding(0)
			, InOwnerTableView);
	}
public: // override SMultiColumnTableRow
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == TEXT("Name"))//判断列名为Type，次名称在创建View时，通过SHeaderRow::Column指定
		{
			return Item->CreateWidget();
		}
		else if (ColumnName == TEXT("DensityMap"))
		{
			return Item->CreateDensityMapPreview();
		}
		else
		{
			return Item->CreateCleanMapPreview();
		}
	}
private:
	TSharedPtr<FSpeceiesModel> Item;
};

TSharedRef<ITableRow> SPCGFoliagePalette::GenerateRowForSpeciesList(TSharedPtr<FSpeceiesModel> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SSpeciesDetailRow, OwnerTable)
		.Item(Item);
	//return	
	//	SNew(STableRow<TSharedPtr<FSpeceiesModel>>, OwnerTable)
	//	.Padding(2.0f)		
	//	[			
	//		SNew(SHorizontalBox)			
	//		+ SHorizontalBox::Slot()
	//		.AutoWidth()
	//		.VAlign(VAlign_Center)
	//		[
	//			Item->CreateWidget()
	//			
	//		]
	//		+ SHorizontalBox::Slot()
	//		.AutoWidth()
	//		.HAlign(HAlign_Center)
	//		.VAlign(VAlign_Center)
	//		[
	//			Item->CreateCleanMapPreview()
	//		]			
	//	];
}
TSharedRef<ITableRow> SPCGFoliagePalette::GenerateRowForBiomeList(TSharedPtr<FBiomeModel> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FBiomeModel>>, OwnerTable)
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
	TArray<UTexture2D*>& CleanMaps = EdMode->GetEditedBiomeData()->CleanMaps;
	TArray<UTextureRenderTarget2D*>& DensityMaps = EdMode->GetEditedBiomeData()->DensityMaps;
	int Num = FMath::Max(CleanMaps.Num(), Species.Num());
	for (int i = 0; i < Num; i++)
	{
		USpecies* SpeciesPtr= i < Species.Num()? Species[i]:nullptr;

		TSharedPtr<FSpeceiesModel> Model = MakeShareable(new FSpeceiesModel(SpeciesPtr, CleanMaps[i], i<DensityMaps.Num()?DensityMaps[i]:nullptr));
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
				.ItemHeight(96)
				.ListItemsSource(&SpeceiesModels)
				.IsEnabled(EdMode,&FRealTimePCGFoliageEdMode::GetPaintSpecies)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("Name")
					.DefaultLabel(FText::FromString(TEXT("Name")))
					+ SHeaderRow::Column("DensityMap")
					.DefaultLabel(FText::FromString(TEXT("DensityMap")))
					+ SHeaderRow::Column("CleanMap")
					.DefaultLabel(FText::FromString(TEXT("CleanMap")))

				)
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
