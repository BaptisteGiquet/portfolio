


#include "UI/Frontend/Screens/EMRKeyRemapScreenWidget.h"

#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "CommonRichTextBlock.h"
#include "ICommonInputModule.h"
#include "LocalizationLibrary.h"
#include "CommonUITypes.h"
#include "Framework/Application/IInputProcessor.h"


class FKeyRemapScreenInputPreProcessor : public IInputProcessor
{

public:
	FKeyRemapScreenInputPreProcessor(ECommonInputType InInputTypeToListenTo, ULocalPlayer* InOwningLocalPlayer)
	: CachedInputTypeToListenTo(InInputTypeToListenTo),
	CachedWeakOwningLocalPlayer(InOwningLocalPlayer)
	{
	}
	
	DECLARE_DELEGATE_OneParam(FOnInputPreProcessorKeyPressedDelegate, const FKey& /*PressedKey*/);
	FOnInputPreProcessorKeyPressedDelegate OnInputPreProcessorKeyPressed;
	
	DECLARE_DELEGATE_OneParam(FOnInputPreProcessorKeySelectCanceledDelegate, const FText& /*CanceledReason*/);
	FOnInputPreProcessorKeySelectCanceledDelegate OnInputPreProcessorKeySelectCanceled;
	
protected:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
			
	}
	
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		ProcessPressedKey(InKeyEvent.GetKey());
		
		return true;
	}
	
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		ProcessPressedKey(MouseEvent.GetEffectingButton());
		
		return true;
	}
	
	void ProcessPressedKey(const FKey& InPressedKey)
	{
		// We filter the escape key and we check if the key is from the right input device
		if (InPressedKey == EKeys::Escape)
		{
			OnInputPreProcessorKeySelectCanceled.ExecuteIfBound(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.RemapMessage.Escape"));
			return;
		}

		UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(CachedWeakOwningLocalPlayer.Get());
		check(CommonInputSubsystem)
		
		ECommonInputType CurrentInputType = CommonInputSubsystem->GetCurrentInputType();
		
		switch (CachedInputTypeToListenTo)
		{
		case ECommonInputType::MouseAndKeyboard:
			
			if (InPressedKey.IsGamepadKey() || CurrentInputType == ECommonInputType::Gamepad)
			{
				OnInputPreProcessorKeySelectCanceled.ExecuteIfBound(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.RemapMessage.WrongDevice"));
				return;
			}
			
			break;
		case ECommonInputType::Gamepad:
			
			if (CurrentInputType == ECommonInputType::Gamepad && InPressedKey == EKeys::LeftMouseButton)
			{
				FCommonInputActionDataBase* InputActionData = ICommonInputModule::GetSettings().GetDefaultClickAction().GetRow<FCommonInputActionDataBase>(TEXT("Crash"));
				check(InputActionData)
				
				OnInputPreProcessorKeyPressed.ExecuteIfBound(InputActionData->GetDefaultGamepadInputTypeInfo().GetKey());
				
				return;
			}
			
			if (!InPressedKey.IsGamepadKey())
			{
				OnInputPreProcessorKeySelectCanceled.ExecuteIfBound(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.RemapMessage.WrongDevice"));
				return;
			}
			
			break;
		default:
			break;
		}
		
		OnInputPreProcessorKeyPressed.ExecuteIfBound(InPressedKey);
	}
	
private:
	ECommonInputType CachedInputTypeToListenTo;
	
	TWeakObjectPtr<ULocalPlayer> CachedWeakOwningLocalPlayer;
};



void UEMRKeyRemapScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	CachedInputPreProcessor = MakeShared<FKeyRemapScreenInputPreProcessor>(CachedDesiredInputType, GetOwningLocalPlayer());
	if (CachedInputPreProcessor)
	{
		CachedInputPreProcessor->OnInputPreProcessorKeyPressed.BindUObject(this, &ThisClass::OnValidKeyPressedDetected);
		CachedInputPreProcessor->OnInputPreProcessorKeySelectCanceled.BindUObject(this, &ThisClass::OnKeySelectCanceled);
		
		FSlateApplication::Get().RegisterInputPreProcessor(CachedInputPreProcessor, -1);	
		
		FText InputDeviceName;
		switch (CachedDesiredInputType)
		{
		case ECommonInputType::MouseAndKeyboard:
			InputDeviceName = GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.MouseAndKeyboard");
			break;
		case ECommonInputType::Gamepad:
			InputDeviceName = GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.Gamepad");
			break;
		default:
			break;
		}
		
		const FText Pattern = GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.RemapMessage.InputDevice");
		const FText Message = FText::Format(Pattern, InputDeviceName);
		
	
		CommonRichTextBlock_RemapMessage->SetText(Message);
	}
	
	
}


void UEMRKeyRemapScreenWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	
	if (CachedInputPreProcessor)
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(CachedInputPreProcessor);
		CachedInputPreProcessor.Reset();
	}
}


void UEMRKeyRemapScreenWidget::OnValidKeyPressedDetected(const FKey& PressedKey)
{
	RequestDeactivateWidget(
		[this, PressedKey]()
		{
			OnKeyRemapScreenKeyPressed.ExecuteIfBound(PressedKey);
		});
}


void UEMRKeyRemapScreenWidget::OnKeySelectCanceled(const FText& CanceledReason)
{
	RequestDeactivateWidget(
		[this, CanceledReason]()
		{
			OnKeyRemapScreenKeySelectCanceled.ExecuteIfBound(CanceledReason);
		});
}


void UEMRKeyRemapScreenWidget::RequestDeactivateWidget(TFunction<void()> PreDeactivateCallback)
{
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda(
			[PreDeactivateCallback, this](float DeltaTime)->bool
			{
				PreDeactivateCallback();
				
				DeactivateWidget();
				
				return false;
			}));
}


void UEMRKeyRemapScreenWidget::SetDesiredInputTypeToFilter(ECommonInputType InDesiredInputType)
{
	CachedDesiredInputType = InDesiredInputType;
}
