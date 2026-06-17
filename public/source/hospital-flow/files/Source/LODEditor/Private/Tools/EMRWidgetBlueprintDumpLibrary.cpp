#include "Tools/EMRWidgetBlueprintDumpLibrary.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ListViewBase.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace EMRWidgetDump
{
	struct FDumpStats
	{
		int32 TotalWidgets = 0;
		int32 PanelWidgets = 0;
		int32 TextWidgets = 0;
		int32 ImageWidgets = 0;
		int32 ProgressBars = 0;
		int32 Sliders = 0;
		int32 Spacers = 0;
		int32 SizeBoxes = 0;
		int32 ScrollBoxes = 0;
		int32 ListViews = 0;
		int32 Invalidations = 0;
		int32 Retainers = 0;
		int32 MaxDepth = 0;
	};

	static TSharedPtr<FJsonObject> MakeErrorPayload(const FString& AssetPath, const FString& Error)
	{
		TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), false);
		Root->SetStringField(TEXT("asset_path"), AssetPath);
		Root->SetStringField(TEXT("error"), Error);
		return Root;
	}

	static FString SerializeJson(const TSharedPtr<FJsonObject>& JsonObject, const bool bPrettyPrint)
	{
		FString OutString;
		if (!JsonObject.IsValid())
		{
			return TEXT("{\"ok\":false,\"error\":\"Invalid JSON object\"}");
		}

		if (bPrettyPrint)
		{
			TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
				TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutString);
			FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
		}
		else
		{
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutString);
			FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
		}

		return OutString;
	}

	static FString EnumValueToString(const UEnum* EnumType, const int64 Value)
	{
		if (!EnumType)
		{
			return FString::FromInt(static_cast<int32>(Value));
		}

		return EnumType->GetNameStringByValue(Value);
	}

	static void IncrementTypeSpecificStats(const UWidget* Widget, FDumpStats& Stats)
	{
		if (Widget == nullptr)
		{
			return;
		}

		if (Cast<const UTextBlock>(Widget))
		{
			++Stats.TextWidgets;
		}
		if (Cast<const UImage>(Widget))
		{
			++Stats.ImageWidgets;
		}
		if (Cast<const UProgressBar>(Widget))
		{
			++Stats.ProgressBars;
		}
		if (Cast<const USlider>(Widget))
		{
			++Stats.Sliders;
		}
		if (Cast<const USpacer>(Widget))
		{
			++Stats.Spacers;
		}
		if (Cast<const USizeBox>(Widget))
		{
			++Stats.SizeBoxes;
		}
		if (Cast<const UScrollBox>(Widget))
		{
			++Stats.ScrollBoxes;
		}
		if (Cast<const UListViewBase>(Widget))
		{
			++Stats.ListViews;
		}

		const FString ClassName = Widget->GetClass()->GetName();
		if (ClassName.Contains(TEXT("InvalidationBox")))
		{
			++Stats.Invalidations;
		}
		if (ClassName.Contains(TEXT("RetainerBox")))
		{
			++Stats.Retainers;
		}
	}

	static void AddCommonWidgetFields(const UWidget* Widget, TSharedPtr<FJsonObject>& Node)
	{
		if (Widget == nullptr || !Node.IsValid())
		{
			return;
		}

		Node->SetStringField(TEXT("name"), Widget->GetName());
		Node->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
		Node->SetStringField(
			TEXT("visibility"),
			EnumValueToString(StaticEnum<ESlateVisibility>(), static_cast<int64>(Widget->GetVisibility())));
		// UE 5.7 exposes ForceVolatile() but not a public volatility getter on UWidget.
		Node->SetBoolField(TEXT("is_volatile"), false);
		Node->SetNumberField(TEXT("render_opacity"), Widget->GetRenderOpacity());
		Node->SetStringField(
			TEXT("clipping"),
			EnumValueToString(StaticEnum<EWidgetClipping>(), static_cast<int64>(Widget->GetClipping())));

		if (const UPanelSlot* PanelSlot = Widget->Slot)
		{
			TSharedPtr<FJsonObject> SlotObject = MakeShared<FJsonObject>();
			SlotObject->SetStringField(TEXT("class"), PanelSlot->GetClass()->GetName());

			if (const UCanvasPanelSlot* CanvasSlot = Cast<const UCanvasPanelSlot>(PanelSlot))
			{
				const FAnchorData Layout = CanvasSlot->GetLayout();
				TSharedPtr<FJsonObject> Canvas = MakeShared<FJsonObject>();
				Canvas->SetNumberField(TEXT("offset_left"), Layout.Offsets.Left);
				Canvas->SetNumberField(TEXT("offset_top"), Layout.Offsets.Top);
				Canvas->SetNumberField(TEXT("offset_right"), Layout.Offsets.Right);
				Canvas->SetNumberField(TEXT("offset_bottom"), Layout.Offsets.Bottom);
				Canvas->SetBoolField(TEXT("auto_size"), CanvasSlot->GetAutoSize());
				SlotObject->SetObjectField(TEXT("canvas"), Canvas);
			}

			Node->SetObjectField(TEXT("slot"), SlotObject);
		}
	}

	static void AddTypeSpecificFields(const UWidget* Widget, TSharedPtr<FJsonObject>& Node)
	{
		if (Widget == nullptr || !Node.IsValid())
		{
			return;
		}

		if (const UTextBlock* TextBlock = Cast<const UTextBlock>(Widget))
		{
			TSharedPtr<FJsonObject> TextObject = MakeShared<FJsonObject>();
			const FString TextValue = TextBlock->GetText().ToString();
			TextObject->SetStringField(TEXT("text_preview"), TextValue.Left(120));
			TextObject->SetNumberField(TEXT("text_length"), TextValue.Len());
			// These properties don't expose stable public getters in UE 5.7.
			// We keep the dump compile-safe and rely on hierarchy/type stats first.
			TextObject->SetBoolField(TEXT("auto_wrap_text"), false);
			TextObject->SetNumberField(TEXT("wrap_text_at"), 0.0);
			TextObject->SetStringField(
				TEXT("overflow_policy"),
				EnumValueToString(StaticEnum<ETextOverflowPolicy>(), static_cast<int64>(TextBlock->GetTextOverflowPolicy())));
			TextObject->SetBoolField(TEXT("simple_text_mode"), false);
			Node->SetObjectField(TEXT("text"), TextObject);
		}
		else if (const UImage* Image = Cast<const UImage>(Widget))
		{
			TSharedPtr<FJsonObject> ImageObject = MakeShared<FJsonObject>();
			const FSlateBrush& Brush = Image->GetBrush();
			ImageObject->SetStringField(
				TEXT("draw_as"),
				EnumValueToString(StaticEnum<ESlateBrushDrawType::Type>(), static_cast<int64>(Brush.DrawAs)));
			ImageObject->SetNumberField(TEXT("image_size_x"), Brush.ImageSize.X);
			ImageObject->SetNumberField(TEXT("image_size_y"), Brush.ImageSize.Y);
			ImageObject->SetBoolField(TEXT("has_resource_object"), Brush.GetResourceObject() != nullptr);
			if (const UObject* ResourceObject = Brush.GetResourceObject())
			{
				ImageObject->SetStringField(TEXT("resource_class"), ResourceObject->GetClass()->GetName());
				ImageObject->SetStringField(TEXT("resource_name"), ResourceObject->GetName());
			}
			Node->SetObjectField(TEXT("image"), ImageObject);
		}
		else if (const UListViewBase* ListView = Cast<const UListViewBase>(Widget))
		{
			TSharedPtr<FJsonObject> ListObject = MakeShared<FJsonObject>();
			ListObject->SetStringField(TEXT("note"), TEXT("ListViewBase details omitted (accessors are limited in UE 5.7 public API)"));
			Node->SetObjectField(TEXT("list_view"), ListObject);
		}
		else if (const UScrollBox* ScrollBox = Cast<const UScrollBox>(Widget))
		{
			TSharedPtr<FJsonObject> ScrollObject = MakeShared<FJsonObject>();
			ScrollObject->SetBoolField(TEXT("allow_overscroll"), ScrollBox->IsAllowOverscroll());
			Node->SetObjectField(TEXT("scroll_box"), ScrollObject);
		}
		else if (const USlider* Slider = Cast<const USlider>(Widget))
		{
			TSharedPtr<FJsonObject> SliderObject = MakeShared<FJsonObject>();
			SliderObject->SetNumberField(TEXT("value"), Slider->GetValue());
			SliderObject->SetBoolField(TEXT("locked"), Slider->IsLocked());
			Node->SetObjectField(TEXT("slider"), SliderObject);
		}
		else if (const UProgressBar* ProgressBar = Cast<const UProgressBar>(Widget))
		{
			TSharedPtr<FJsonObject> ProgressObject = MakeShared<FJsonObject>();
			ProgressObject->SetNumberField(TEXT("percent"), ProgressBar->GetPercent());
			ProgressObject->SetBoolField(TEXT("is_marquee"), ProgressBar->UseMarquee());
			Node->SetObjectField(TEXT("progress_bar"), ProgressObject);
		}
	}

	static TSharedPtr<FJsonObject> DumpWidgetNodeRecursive(const UWidget* Widget, const int32 Depth, FDumpStats& Stats)
	{
		if (Widget == nullptr)
		{
			return nullptr;
		}

		++Stats.TotalWidgets;
		Stats.MaxDepth = FMath::Max(Stats.MaxDepth, Depth);
		if (Cast<const UPanelWidget>(Widget))
		{
			++Stats.PanelWidgets;
		}
		IncrementTypeSpecificStats(Widget, Stats);

		TSharedPtr<FJsonObject> Node = MakeShared<FJsonObject>();
		Node->SetNumberField(TEXT("depth"), Depth);
		AddCommonWidgetFields(Widget, Node);
		AddTypeSpecificFields(Widget, Node);

		TArray<TSharedPtr<FJsonValue>> ChildrenJson;
		if (const UPanelWidget* PanelWidget = Cast<const UPanelWidget>(Widget))
		{
			const int32 ChildCount = PanelWidget->GetChildrenCount();
			Node->SetNumberField(TEXT("child_count"), ChildCount);
			for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
			{
				if (const UWidget* ChildWidget = PanelWidget->GetChildAt(ChildIndex))
				{
					if (TSharedPtr<FJsonObject> ChildNode = DumpWidgetNodeRecursive(ChildWidget, Depth + 1, Stats))
					{
						ChildNode->SetNumberField(TEXT("child_index"), ChildIndex);
						ChildrenJson.Add(MakeShared<FJsonValueObject>(ChildNode));
					}
				}
			}
		}
		else
		{
			Node->SetNumberField(TEXT("child_count"), 0);
		}

		Node->SetArrayField(TEXT("children"), ChildrenJson);
		return Node;
	}

	static UObject* TryLoadAsset(const FString& InputPath)
	{
		if (InputPath.IsEmpty())
		{
			return nullptr;
		}

		if (UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *InputPath))
		{
			return Asset;
		}

		if (InputPath.StartsWith(TEXT("/Game/")) && !InputPath.Contains(TEXT(".")))
		{
			const FString AssetName = FPaths::GetBaseFilename(InputPath);
			const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *InputPath, *AssetName);
			return StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
		}

		return nullptr;
	}

	static FString ResolveOutputPath(const FString& InputPath)
	{
		if (InputPath.IsEmpty())
		{
			return FString();
		}

		if (FPaths::IsRelative(InputPath))
		{
			return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / InputPath);
		}

		return FPaths::ConvertRelativePathToFull(InputPath);
	}

	static TSharedPtr<FJsonObject> BuildDumpPayload(const FString& AssetPath)
	{
		UObject* AssetObject = TryLoadAsset(AssetPath);
		if (AssetObject == nullptr)
		{
			return MakeErrorPayload(AssetPath, TEXT("Failed to load asset. Expected a Widget Blueprint asset path."));
		}

		const UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(AssetObject);
		if (WidgetBlueprint == nullptr)
		{
			return MakeErrorPayload(AssetPath, FString::Printf(TEXT("Asset is not a UWidgetBlueprint (got %s)."), *AssetObject->GetClass()->GetName()));
		}

		const UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(WidgetBlueprint->GeneratedClass);
		if (WidgetClass == nullptr)
		{
			return MakeErrorPayload(AssetPath, TEXT("Widget Blueprint has no generated UWidgetBlueprintGeneratedClass."));
		}

		const UWidgetBlueprintGeneratedClass* TreeOwningClass = WidgetClass->FindWidgetTreeOwningClass();
		const UWidgetBlueprintGeneratedClass* EffectiveTreeClass = TreeOwningClass ? TreeOwningClass : WidgetClass;
		const UWidgetTree* WidgetTree = EffectiveTreeClass->GetWidgetTreeArchetype();
		if (WidgetTree == nullptr)
		{
			return MakeErrorPayload(AssetPath, TEXT("Generated class has no WidgetTree archetype."));
		}

		const UWidget* RootWidget = WidgetTree->RootWidget;
		if (RootWidget == nullptr)
		{
			return MakeErrorPayload(AssetPath, TEXT("WidgetTree has no RootWidget."));
		}

		FDumpStats Stats;
		TSharedPtr<FJsonObject> TreeNode = DumpWidgetNodeRecursive(RootWidget, 0, Stats);

		TSharedPtr<FJsonObject> StatsObject = MakeShared<FJsonObject>();
		StatsObject->SetNumberField(TEXT("total_widgets"), Stats.TotalWidgets);
		StatsObject->SetNumberField(TEXT("panel_widgets"), Stats.PanelWidgets);
		StatsObject->SetNumberField(TEXT("text_widgets"), Stats.TextWidgets);
		StatsObject->SetNumberField(TEXT("image_widgets"), Stats.ImageWidgets);
		StatsObject->SetNumberField(TEXT("progress_bars"), Stats.ProgressBars);
		StatsObject->SetNumberField(TEXT("sliders"), Stats.Sliders);
		StatsObject->SetNumberField(TEXT("spacers"), Stats.Spacers);
		StatsObject->SetNumberField(TEXT("size_boxes"), Stats.SizeBoxes);
		StatsObject->SetNumberField(TEXT("scroll_boxes"), Stats.ScrollBoxes);
		StatsObject->SetNumberField(TEXT("list_views"), Stats.ListViews);
		StatsObject->SetNumberField(TEXT("invalidation_boxes"), Stats.Invalidations);
		StatsObject->SetNumberField(TEXT("retainer_boxes"), Stats.Retainers);
		StatsObject->SetNumberField(TEXT("max_depth"), Stats.MaxDepth);

		TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), true);
		Root->SetStringField(TEXT("asset_path"), WidgetBlueprint->GetPathName());
		Root->SetStringField(TEXT("blueprint_name"), WidgetBlueprint->GetName());
		Root->SetStringField(TEXT("generated_class"), GetNameSafe(WidgetClass));
		Root->SetStringField(TEXT("tree_owning_class"), GetNameSafe(EffectiveTreeClass));
		Root->SetNumberField(TEXT("declared_binding_count"), WidgetClass->Bindings.Num());
		Root->SetNumberField(TEXT("declared_animation_count"), WidgetClass->Animations.Num());
		Root->SetObjectField(TEXT("stats"), StatsObject);
		Root->SetObjectField(TEXT("tree"), TreeNode);

		return Root;
	}
}

FString UEMRWidgetBlueprintDumpLibrary::ExportWidgetBlueprintTreeJson(const FString& WidgetBlueprintAssetPath, const bool bPrettyPrint)
{
	return EMRWidgetDump::SerializeJson(EMRWidgetDump::BuildDumpPayload(WidgetBlueprintAssetPath), bPrettyPrint);
}

FString UEMRWidgetBlueprintDumpLibrary::SaveWidgetBlueprintTreeJson(const FString& WidgetBlueprintAssetPath, const FString& OutputPath, const bool bPrettyPrint)
{
	TSharedPtr<FJsonObject> Payload = EMRWidgetDump::BuildDumpPayload(WidgetBlueprintAssetPath);
	FString JsonString = EMRWidgetDump::SerializeJson(Payload, bPrettyPrint);

	const FString ResolvedPath = EMRWidgetDump::ResolveOutputPath(OutputPath);
	if (ResolvedPath.IsEmpty())
	{
		return EMRWidgetDump::SerializeJson(
			EMRWidgetDump::MakeErrorPayload(WidgetBlueprintAssetPath, TEXT("OutputPath is empty.")),
			bPrettyPrint);
	}

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(ResolvedPath), true);
	if (!FFileHelper::SaveStringToFile(JsonString, *ResolvedPath))
	{
		return EMRWidgetDump::SerializeJson(
			EMRWidgetDump::MakeErrorPayload(WidgetBlueprintAssetPath, FString::Printf(TEXT("Failed to write file: %s"), *ResolvedPath)),
			bPrettyPrint);
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("ok"), true);
	Result->SetStringField(TEXT("asset_path"), WidgetBlueprintAssetPath);
	Result->SetStringField(TEXT("output_path"), ResolvedPath);
	Result->SetNumberField(TEXT("json_length"), JsonString.Len());
	return EMRWidgetDump::SerializeJson(Result, bPrettyPrint);
}

FString UEMRWidgetBlueprintDumpLibrary::ReplaceSliderWithProgressBar(
	const FString& WidgetBlueprintAssetPath,
	const FString& SliderWidgetName,
	const FString& ProgressBarWidgetName,
	const bool bCompileBlueprint)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), WidgetBlueprintAssetPath);
	Result->SetStringField(TEXT("slider_widget_name"), SliderWidgetName);
	Result->SetStringField(TEXT("progress_bar_widget_name"), ProgressBarWidgetName);

	if (SliderWidgetName.IsEmpty() || ProgressBarWidgetName.IsEmpty())
	{
		Result->SetBoolField(TEXT("ok"), false);
		Result->SetStringField(TEXT("error"), TEXT("SliderWidgetName and ProgressBarWidgetName must be non-empty."));
		return EMRWidgetDump::SerializeJson(Result, true);
	}

	UObject* AssetObject = EMRWidgetDump::TryLoadAsset(WidgetBlueprintAssetPath);
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(AssetObject);
	if (WidgetBlueprint == nullptr)
	{
		Result->SetBoolField(TEXT("ok"), false);
		Result->SetStringField(TEXT("error"), TEXT("Failed to load UWidgetBlueprint asset."));
		if (AssetObject)
		{
			Result->SetStringField(TEXT("asset_class"), AssetObject->GetClass()->GetName());
		}
		return EMRWidgetDump::SerializeJson(Result, true);
	}

	UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(WidgetBlueprint->GeneratedClass);
	if (WidgetClass == nullptr)
	{
		Result->SetBoolField(TEXT("ok"), false);
		Result->SetStringField(TEXT("error"), TEXT("WidgetBlueprint has no generated UWidgetBlueprintGeneratedClass."));
		return EMRWidgetDump::SerializeJson(Result, true);
	}

	UWidgetBlueprintGeneratedClass* TreeOwningClass = WidgetClass->FindWidgetTreeOwningClass();
	UWidgetBlueprintGeneratedClass* EffectiveTreeClass = TreeOwningClass ? TreeOwningClass : WidgetClass;
	UWidgetTree* WidgetTree = EffectiveTreeClass->GetWidgetTreeArchetype();
	if (WidgetTree == nullptr)
	{
		Result->SetBoolField(TEXT("ok"), false);
		Result->SetStringField(TEXT("error"), TEXT("Generated class has no widget tree archetype."));
		return EMRWidgetDump::SerializeJson(Result, true);
	}

	UWidget* OldWidget = WidgetTree->FindWidget(*SliderWidgetName);
	if (OldWidget == nullptr)
	{
		Result->SetBoolField(TEXT("ok"), false);
		Result->SetStringField(TEXT("error"), TEXT("Slider widget not found in widget tree."));
		return EMRWidgetDump::SerializeJson(Result, true);
	}

	USlider* OldSlider = Cast<USlider>(OldWidget);
	if (OldSlider == nullptr)
	{
		Result->SetBoolField(TEXT("ok"), false);
		Result->SetStringField(TEXT("error"), FString::Printf(TEXT("Widget '%s' is not a USlider/UAnalogSlider (class=%s)."), *SliderWidgetName, *OldWidget->GetClass()->GetName()));
		return EMRWidgetDump::SerializeJson(Result, true);
	}

	UWidget* ExistingProgressWidget = WidgetTree->FindWidget(*ProgressBarWidgetName);
	if (ExistingProgressWidget)
	{
		Result->SetBoolField(TEXT("ok"), false);
		Result->SetStringField(TEXT("error"), FString::Printf(TEXT("Target progress bar name '%s' already exists in widget tree."), *ProgressBarWidgetName));
		return EMRWidgetDump::SerializeJson(Result, true);
	}

	WidgetBlueprint->Modify();
	WidgetTree->Modify();
	OldWidget->Modify();

	UProgressBar* NewProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), *ProgressBarWidgetName);
	if (NewProgressBar == nullptr)
	{
		Result->SetBoolField(TEXT("ok"), false);
		Result->SetStringField(TEXT("error"), TEXT("Failed to construct UProgressBar in widget tree."));
		return EMRWidgetDump::SerializeJson(Result, true);
	}

	NewProgressBar->Modify();
	NewProgressBar->SetVisibility(OldWidget->GetVisibility());
	NewProgressBar->SetRenderOpacity(OldWidget->GetRenderOpacity());
	NewProgressBar->SetClipping(OldWidget->GetClipping());
	NewProgressBar->SetIsEnabled(OldWidget->GetIsEnabled());
	NewProgressBar->SetPercent(FMath::Clamp(OldSlider->GetValue(), 0.0f, 1.0f));

	bool bReplaced = false;
	if (OldWidget == WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = NewProgressBar;
		bReplaced = true;
	}
	else if (UPanelWidget* ParentPanel = OldWidget->GetParent())
	{
		ParentPanel->Modify();
		bReplaced = ParentPanel->ReplaceChild(OldWidget, NewProgressBar);
	}

	if (!bReplaced)
	{
		Result->SetBoolField(TEXT("ok"), false);
		Result->SetStringField(TEXT("error"), TEXT("Failed to replace widget in parent panel/root."));
		return EMRWidgetDump::SerializeJson(Result, true);
	}

	// ReplaceChild does not clear the previous child's slot pointer in editor code path.
	OldWidget->Slot = nullptr;

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	if (bCompileBlueprint)
	{
		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	}

	Result->SetBoolField(TEXT("ok"), true);
	Result->SetStringField(TEXT("replaced_widget_class"), OldWidget->GetClass()->GetName());
	Result->SetStringField(TEXT("new_widget_class"), NewProgressBar->GetClass()->GetName());
	Result->SetNumberField(TEXT("initial_percent"), NewProgressBar->GetPercent());
	return EMRWidgetDump::SerializeJson(Result, true);
}
