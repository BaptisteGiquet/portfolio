#include "Commands/UnrealMCPCommonUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UObjectIterator.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"
#include "Engine/Selection.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabase.h"
#include "Misc/PackageName.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// JSON Utilities
TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateErrorResponse(const FString& Message)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), false);
    ResponseObject->SetStringField(TEXT("error"), Message);
    return ResponseObject;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), true);
    
    if (Data.IsValid())
    {
        ResponseObject->SetObjectField(TEXT("data"), Data);
    }
    
    return ResponseObject;
}

void FUnrealMCPCommonUtils::GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((int32)Value->AsNumber());
        }
    }
}

void FUnrealMCPCommonUtils::GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((float)Value->AsNumber());
        }
    }
}

FVector2D FUnrealMCPCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector2D Result(0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 2)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
    }
    
    return Result;
}

FVector FUnrealMCPCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
        Result.Z = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

FRotator FUnrealMCPCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FRotator Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.Pitch = (float)(*JsonArray)[0]->AsNumber();
        Result.Yaw = (float)(*JsonArray)[1]->AsNumber();
        Result.Roll = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

// Blueprint Utilities
UBlueprint* FUnrealMCPCommonUtils::FindBlueprint(const FString& BlueprintName)
{
    return FindBlueprintByName(BlueprintName);
}

UBlueprint* FUnrealMCPCommonUtils::FindBlueprintByName(const FString& BlueprintName)
{
    FString Query = BlueprintName;
    Query.TrimStartAndEndInline();
    if (Query.IsEmpty())
    {
        return nullptr;
    }

    // Handle accidental file-system style inputs.
    if (Query.EndsWith(TEXT(".uasset")))
    {
        Query = Query.LeftChop(7);
    }

    TArray<FString> CandidateObjectPaths;
    auto AddCandidate = [&CandidateObjectPaths](const FString& InPath)
    {
        FString Normalized = InPath;
        while (Normalized.ReplaceInline(TEXT("//"), TEXT("/"), ESearchCase::CaseSensitive) > 0)
        {
        }

        if (!Normalized.IsEmpty() && !CandidateObjectPaths.Contains(Normalized))
        {
            CandidateObjectPaths.Add(Normalized);
        }
    };

    const auto EnsureObjectPath = [](const FString& Path) -> FString
    {
        if (Path.Contains(TEXT(".")))
        {
            return Path;
        }

        const FString AssetName = FPackageName::GetShortName(Path);
        return AssetName.IsEmpty() ? Path : (Path + TEXT(".") + AssetName);
    };

    if (Query.StartsWith(TEXT("/")))
    {
        AddCandidate(EnsureObjectPath(Query));
    }
    else
    {
        // Legacy lookup under /Game/Blueprints
        AddCandidate(EnsureObjectPath(TEXT("/Game/Blueprints/") + Query));

        // Also support queries relative to /Game
        AddCandidate(EnsureObjectPath(TEXT("/Game/") + Query));
    }

    for (const FString& ObjectPath : CandidateObjectPaths)
    {
        if (UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath))
        {
            return Blueprint;
        }
    }

    // Fallback search by asset name in /Game to support plain short names.
    const FString TargetAssetName = FPackageName::GetShortName(Query);
    if (!TargetAssetName.IsEmpty())
    {
        FAssetRegistryModule& AssetRegistryModule =
            FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

        FARFilter Filter;
        Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
        Filter.PackagePaths.Add(TEXT("/Game"));
        Filter.bRecursivePaths = true;

        TArray<FAssetData> BlueprintAssets;
        AssetRegistryModule.Get().GetAssets(Filter, BlueprintAssets);
        for (const FAssetData& AssetData : BlueprintAssets)
        {
            if (AssetData.AssetName.ToString().Equals(TargetAssetName, ESearchCase::IgnoreCase))
            {
                if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
                {
                    return Blueprint;
                }
            }
        }
    }

    return nullptr;
}

UEdGraph* FUnrealMCPCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Try to find the event graph
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetName().Contains(TEXT("EventGraph")))
        {
            return Graph;
        }
    }
    
    // Create a new event graph if none exists
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(TEXT("EventGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
    return NewGraph;
}

// Blueprint node utilities
UK2Node_Event* FUnrealMCPCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Check for existing event node with this exact name
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Using existing event node with name %s (ID: %s)"), 
                *EventName, *EventNode->NodeGuid.ToString());
            return EventNode;
        }
    }

    // No existing node found, create a new one
    UK2Node_Event* EventNode = nullptr;
    
    // Find the function to create the event
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    UFunction* EventFunction = BlueprintClass->FindFunctionByName(FName(*EventName));
    
    if (EventFunction)
    {
        EventNode = NewObject<UK2Node_Event>(Graph);
        EventNode->EventReference.SetExternalMember(FName(*EventName), BlueprintClass);
        EventNode->NodePosX = Position.X;
        EventNode->NodePosY = Position.Y;
        Graph->AddNode(EventNode, true);
        EventNode->PostPlacedNewNode();
        EventNode->AllocateDefaultPins();
        UE_LOG(LogTemp, Display, TEXT("Created new event node with name %s (ID: %s)"), 
            *EventName, *EventNode->NodeGuid.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find function for event name: %s"), *EventName);
    }
    
    return EventNode;
}

UK2Node_CallFunction* FUnrealMCPCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position)
{
    if (!Graph || !Function)
    {
        return nullptr;
    }
    
    UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
    FunctionNode->SetFromFunction(Function);
    FunctionNode->NodePosX = Position.X;
    FunctionNode->NodePosY = Position.Y;
    Graph->AddNode(FunctionNode, true);
    FunctionNode->CreateNewGuid();
    FunctionNode->PostPlacedNewNode();
    FunctionNode->AllocateDefaultPins();
    
    return FunctionNode;
}

UK2Node_VariableGet* FUnrealMCPCommonUtils::CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableGet* VariableGetNode = NewObject<UK2Node_VariableGet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableGetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableGetNode->NodePosX = Position.X;
        VariableGetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableGetNode, true);
        VariableGetNode->PostPlacedNewNode();
        VariableGetNode->AllocateDefaultPins();
        
        return VariableGetNode;
    }
    
    return nullptr;
}

UK2Node_VariableSet* FUnrealMCPCommonUtils::CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableSet* VariableSetNode = NewObject<UK2Node_VariableSet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableSetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableSetNode->NodePosX = Position.X;
        VariableSetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableSetNode, true);
        VariableSetNode->PostPlacedNewNode();
        VariableSetNode->AllocateDefaultPins();
        
        return VariableSetNode;
    }
    
    return nullptr;
}

UK2Node_InputAction* FUnrealMCPCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
    InputActionNode->InputActionName = FName(*ActionName);
    InputActionNode->NodePosX = Position.X;
    InputActionNode->NodePosY = Position.Y;
    Graph->AddNode(InputActionNode, true);
    InputActionNode->CreateNewGuid();
    InputActionNode->PostPlacedNewNode();
    InputActionNode->AllocateDefaultPins();
    
    return InputActionNode;
}

UK2Node_Self* FUnrealMCPCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
    SelfNode->NodePosX = Position.X;
    SelfNode->NodePosY = Position.Y;
    Graph->AddNode(SelfNode, true);
    SelfNode->CreateNewGuid();
    SelfNode->PostPlacedNewNode();
    SelfNode->AllocateDefaultPins();
    
    return SelfNode;
}

bool FUnrealMCPCommonUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                           UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        return false;
    }
    
    UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);
    
    if (SourcePin && TargetPin)
    {
        SourcePin->MakeLinkTo(TargetPin);
        return true;
    }
    
    return false;
}

UEdGraphPin* FUnrealMCPCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }
    
    // Log all pins for debugging
    UE_LOG(LogTemp, Display, TEXT("FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"), 
           *PinName, (int32)Direction, *Node->GetName());
    
    for (UEdGraphPin* Pin : Node->Pins)
    {
        UE_LOG(LogTemp, Display, TEXT("  - Available pin: '%s', Direction: %d, Category: %s"), 
               *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
    }
    
    // First try exact match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found exact matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If no exact match and we're looking for a component reference, try case-insensitive match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) && 
            (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If we're looking for a component output and didn't find it by name, try to find the first data output pin
    if (Direction == EGPD_Output && Cast<UK2Node_VariableGet>(Node) != nullptr)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                UE_LOG(LogTemp, Display, TEXT("  - Found fallback data output pin: '%s'"), *Pin->PinName.ToString());
                return Pin;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("  - No matching pin found for '%s'"), *PinName);
    return nullptr;
}

// Actor utilities
TSharedPtr<FJsonValue> FUnrealMCPCommonUtils::ActorToJson(AActor* Actor)
{
    if (!Actor)
    {
        return MakeShared<FJsonValueNull>();
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return MakeShared<FJsonValueObject>(ActorObject);
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::ActorToJsonObject(AActor* Actor, bool bDetailed)
{
    if (!Actor)
    {
        return nullptr;
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return ActorObject;
}

UK2Node_Event* FUnrealMCPCommonUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
    if (!Graph)
    {
        return nullptr;
    }

    // Look for existing event nodes
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Found existing event node with name: %s"), *EventName);
            return EventNode;
        }
    }

    return nullptr;
}

namespace
{
    bool ResolvePropertyPath(
        UObject* RootObject,
        const FString& PropertyPath,
        FProperty*& OutProperty,
        void*& OutPropertyAddress,
        FString& OutErrorMessage);

    bool SetPropertyValueFromJson(FProperty* Property, void* PropertyAddress, const TSharedPtr<FJsonValue>& JsonValue, FString& OutErrorMessage);
}

bool FUnrealMCPCommonUtils::SetObjectProperty(UObject* Object, const FString& PropertyName, 
                                     const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
    if (!Object)
    {
        OutErrorMessage = TEXT("Invalid object");
        return false;
    }

    FProperty* ResolvedProperty = nullptr;
    void* ResolvedPropertyAddress = nullptr;
    if (!ResolvePropertyPath(Object, PropertyName, ResolvedProperty, ResolvedPropertyAddress, OutErrorMessage))
    {
        return false;
    }

    return SetPropertyValueFromJson(ResolvedProperty, ResolvedPropertyAddress, Value, OutErrorMessage);
}

bool FUnrealMCPCommonUtils::HasObjectProperty(UObject* Object, const FString& PropertyName)
{
    FString ErrorMessage;
    FProperty* Property = nullptr;
    void* PropertyAddress = nullptr;
    return ResolvePropertyPath(Object, PropertyName, Property, PropertyAddress, ErrorMessage);
}

UActorComponent* FUnrealMCPCommonUtils::FindActorComponent(AActor* Actor, const FString& ComponentQuery, FString& OutErrorMessage)
{
    if (!Actor)
    {
        OutErrorMessage = TEXT("Invalid actor");
        return nullptr;
    }

    FString Query = ComponentQuery;
    Query.TrimStartAndEndInline();
    const FString QueryWithoutUPrefix = Query.StartsWith(TEXT("U")) ? Query.Mid(1) : Query;

    if (Query.IsEmpty() || Query.Equals(TEXT("Root"), ESearchCase::IgnoreCase) || Query.Equals(TEXT("RootComponent"), ESearchCase::IgnoreCase))
    {
        return Actor->GetRootComponent();
    }

    TInlineComponentArray<UActorComponent*> Components(Actor);
    TArray<UActorComponent*> ExactMatches;
    TArray<UActorComponent*> PartialMatches;

    auto MatchesClassName = [&Query, &QueryWithoutUPrefix](const UActorComponent* Component) -> bool
    {
        const FString ClassName = Component->GetClass()->GetName();
        const FString ClassWithoutUPrefix = ClassName.StartsWith(TEXT("U")) ? ClassName.Mid(1) : ClassName;
        return ClassName.Equals(Query, ESearchCase::IgnoreCase) ||
            ClassWithoutUPrefix.Equals(Query, ESearchCase::IgnoreCase) ||
            ClassName.Equals(QueryWithoutUPrefix, ESearchCase::IgnoreCase) ||
            ClassWithoutUPrefix.Equals(QueryWithoutUPrefix, ESearchCase::IgnoreCase);
    };

    for (UActorComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

        const FString ComponentName = Component->GetName();
        const bool bExactNameMatch = ComponentName.Equals(Query, ESearchCase::IgnoreCase);
        const bool bExactPathMatch = Component->GetPathName().Equals(Query, ESearchCase::IgnoreCase);
        const bool bExactClassMatch = MatchesClassName(Component);

        if (bExactNameMatch || bExactPathMatch || bExactClassMatch)
        {
            ExactMatches.Add(Component);
            continue;
        }

        const bool bPartialMatch =
            ComponentName.Contains(Query, ESearchCase::IgnoreCase) ||
            Query.Contains(ComponentName, ESearchCase::IgnoreCase) ||
            Component->GetClass()->GetName().Contains(Query, ESearchCase::IgnoreCase);

        if (bPartialMatch)
        {
            PartialMatches.Add(Component);
        }
    }

    const TArray<UActorComponent*>& CandidateMatches = ExactMatches.Num() > 0 ? ExactMatches : PartialMatches;
    if (CandidateMatches.Num() == 1)
    {
        return CandidateMatches[0];
    }

    if (CandidateMatches.Num() > 1)
    {
        FString CandidateDescription;
        for (UActorComponent* Candidate : CandidateMatches)
        {
            if (!CandidateDescription.IsEmpty())
            {
                CandidateDescription += TEXT(", ");
            }
            CandidateDescription += FString::Printf(TEXT("%s(%s)"), *Candidate->GetName(), *Candidate->GetClass()->GetName());
        }

        OutErrorMessage = FString::Printf(
            TEXT("Component query '%s' is ambiguous on actor '%s'. Matches: %s"),
            *ComponentQuery,
            *Actor->GetName(),
            *CandidateDescription);
        return nullptr;
    }

    FString AvailableComponents;
    for (UActorComponent* Component : Components)
    {
        if (!AvailableComponents.IsEmpty())
        {
            AvailableComponents += TEXT(", ");
        }
        AvailableComponents += FString::Printf(TEXT("%s(%s)"), *Component->GetName(), *Component->GetClass()->GetName());
    }

    OutErrorMessage = FString::Printf(
        TEXT("Component '%s' not found on actor '%s'. Available components: %s"),
        *ComponentQuery,
        *Actor->GetName(),
        *AvailableComponents);
    return nullptr;
}

namespace
{
    struct FMCPPropertyPathSegment
    {
        FString Name;
        bool bHasArrayIndex = false;
        int32 ArrayIndex = INDEX_NONE;
    };

    bool TryGetStringLikeValue(const TSharedPtr<FJsonValue>& JsonValue, FString& OutString)
    {
        if (!JsonValue.IsValid())
        {
            return false;
        }

        if (JsonValue->Type == EJson::String)
        {
            OutString = JsonValue->AsString();
            return true;
        }

        if (JsonValue->Type == EJson::Number)
        {
            OutString = LexToString(JsonValue->AsNumber());
            return true;
        }

        if (JsonValue->Type == EJson::Boolean)
        {
            OutString = JsonValue->AsBool() ? TEXT("true") : TEXT("false");
            return true;
        }

        return false;
    }

    bool IsJsonNullLike(const TSharedPtr<FJsonValue>& JsonValue)
    {
        if (!JsonValue.IsValid() || JsonValue->Type == EJson::Null || JsonValue->Type == EJson::None)
        {
            return true;
        }

        if (JsonValue->Type == EJson::String)
        {
            const FString Value = JsonValue->AsString().TrimStartAndEnd();
            return Value.IsEmpty() || Value.Equals(TEXT("None"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("null"), ESearchCase::IgnoreCase);
        }

        return false;
    }

    bool ParsePropertyPathSegment(const FString& RawSegment, FMCPPropertyPathSegment& OutSegment)
    {
        OutSegment = FMCPPropertyPathSegment();
        const FString Segment = RawSegment.TrimStartAndEnd();
        if (Segment.IsEmpty())
        {
            return false;
        }

        const int32 OpenBracketIndex = Segment.Find(TEXT("["));
        if (OpenBracketIndex == INDEX_NONE)
        {
            OutSegment.Name = Segment;
            return true;
        }

        const int32 CloseBracketIndex = Segment.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, OpenBracketIndex + 1);
        if (CloseBracketIndex == INDEX_NONE || CloseBracketIndex != Segment.Len() - 1)
        {
            return false;
        }

        OutSegment.Name = Segment.Left(OpenBracketIndex).TrimStartAndEnd();
        const FString IndexString = Segment.Mid(OpenBracketIndex + 1, CloseBracketIndex - OpenBracketIndex - 1).TrimStartAndEnd();
        if (OutSegment.Name.IsEmpty() || IndexString.IsEmpty() || !IndexString.IsNumeric())
        {
            return false;
        }

        OutSegment.bHasArrayIndex = true;
        OutSegment.ArrayIndex = FCString::Atoi(*IndexString);
        return OutSegment.ArrayIndex >= 0;
    }

    FString JsonTypeToString(const TSharedPtr<FJsonValue>& JsonValue)
    {
        if (!JsonValue.IsValid())
        {
            return TEXT("Null");
        }

        switch (JsonValue->Type)
        {
        case EJson::None: return TEXT("None");
        case EJson::Null: return TEXT("Null");
        case EJson::String: return TEXT("String");
        case EJson::Number: return TEXT("Number");
        case EJson::Boolean: return TEXT("Boolean");
        case EJson::Array: return TEXT("Array");
        case EJson::Object: return TEXT("Object");
        default: return TEXT("Unknown");
        }
    }

    bool TryGetJsonNumber(const TSharedPtr<FJsonValue>& JsonValue, double& OutNumber)
    {
        if (!JsonValue.IsValid())
        {
            return false;
        }

        if (JsonValue->Type == EJson::Number)
        {
            OutNumber = JsonValue->AsNumber();
            return true;
        }

        if (JsonValue->Type == EJson::Boolean)
        {
            OutNumber = JsonValue->AsBool() ? 1.0 : 0.0;
            return true;
        }

        if (JsonValue->Type == EJson::String)
        {
            const FString StringValue = JsonValue->AsString().TrimStartAndEnd();
            if (StringValue.IsNumeric())
            {
                OutNumber = FCString::Atod(*StringValue);
                return true;
            }
        }

        return false;
    }

    bool TryGetJsonBool(const TSharedPtr<FJsonValue>& JsonValue, bool& OutBool)
    {
        if (!JsonValue.IsValid())
        {
            return false;
        }

        if (JsonValue->Type == EJson::Boolean)
        {
            OutBool = JsonValue->AsBool();
            return true;
        }

        if (JsonValue->Type == EJson::Number)
        {
            OutBool = FMath::Abs(JsonValue->AsNumber()) > KINDA_SMALL_NUMBER;
            return true;
        }

        if (JsonValue->Type == EJson::String)
        {
            FString StringValue = JsonValue->AsString().TrimStartAndEnd().ToLower();
            if (StringValue == TEXT("true") || StringValue == TEXT("1") || StringValue == TEXT("yes") || StringValue == TEXT("on"))
            {
                OutBool = true;
                return true;
            }

            if (StringValue == TEXT("false") || StringValue == TEXT("0") || StringValue == TEXT("no") || StringValue == TEXT("off"))
            {
                OutBool = false;
                return true;
            }
        }

        return false;
    }

    bool TryGetNumberFieldCaseInsensitive(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, double& OutNumber)
    {
        if (!JsonObject.IsValid())
        {
            return false;
        }

        const TSharedPtr<FJsonValue>* ValuePtr = JsonObject->Values.Find(FieldName);
        if (ValuePtr && TryGetJsonNumber(*ValuePtr, OutNumber))
        {
            return true;
        }

        const FString TargetField = FString(FieldName);
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : JsonObject->Values)
        {
            if (Field.Key.Equals(TargetField, ESearchCase::IgnoreCase) && TryGetJsonNumber(Field.Value, OutNumber))
            {
                return true;
            }
        }

        return false;
    }

    FString NormalizeBoolPropertyName(const FString& PropertyName)
    {
        FString Normalized = PropertyName.TrimStartAndEnd();
        if (Normalized.StartsWith(TEXT("b")) && Normalized.Len() > 1 && FChar::IsUpper(Normalized[1]))
        {
            return Normalized.Mid(1);
        }

        return Normalized;
    }

    FProperty* FindPropertyFlexible(UStruct* ContainerStruct, const FString& PropertyName)
    {
        if (!ContainerStruct)
        {
            return nullptr;
        }

        const FString Requested = PropertyName.TrimStartAndEnd();
        if (Requested.IsEmpty())
        {
            return nullptr;
        }

        if (FProperty* Exact = ContainerStruct->FindPropertyByName(*Requested))
        {
            return Exact;
        }

        FProperty* CaseInsensitiveMatch = nullptr;
        const FString RequestedNormalized = NormalizeBoolPropertyName(Requested);

        for (TFieldIterator<FProperty> It(ContainerStruct, EFieldIterationFlags::IncludeSuper); It; ++It)
        {
            FProperty* Candidate = *It;
            if (!Candidate)
            {
                continue;
            }

            const FString CandidateName = Candidate->GetName();
            if (CandidateName.Equals(Requested, ESearchCase::IgnoreCase))
            {
                return Candidate;
            }

            if (CaseInsensitiveMatch == nullptr)
            {
                const FString CandidateNormalized = NormalizeBoolPropertyName(CandidateName);
                if (CandidateNormalized.Equals(RequestedNormalized, ESearchCase::IgnoreCase))
                {
                    CaseInsensitiveMatch = Candidate;
                }
            }
        }

        return CaseInsensitiveMatch;
    }

    bool ResolvePropertyPath(
        UObject* RootObject,
        const FString& PropertyPath,
        FProperty*& OutProperty,
        void*& OutPropertyAddress,
        FString& OutErrorMessage)
    {
        OutProperty = nullptr;
        OutPropertyAddress = nullptr;

        if (!RootObject)
        {
            OutErrorMessage = TEXT("Invalid object");
            return false;
        }

        const FString SanitizedPath = PropertyPath.TrimStartAndEnd();
        if (SanitizedPath.IsEmpty())
        {
            OutErrorMessage = TEXT("Property path is empty");
            return false;
        }

        TArray<FString> Segments;
        SanitizedPath.ParseIntoArray(Segments, TEXT("."), true);
        if (Segments.Num() == 0)
        {
            OutErrorMessage = TEXT("Property path is empty");
            return false;
        }

        UStruct* CurrentStruct = RootObject->GetClass();
        void* CurrentContainer = RootObject;

        for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
        {
            FMCPPropertyPathSegment ParsedSegment;
            if (!ParsePropertyPathSegment(Segments[SegmentIndex], ParsedSegment))
            {
                OutErrorMessage = FString::Printf(TEXT("Invalid property segment syntax: %s"), *Segments[SegmentIndex]);
                return false;
            }

            FProperty* Property = FindPropertyFlexible(CurrentStruct, ParsedSegment.Name);
            if (!Property)
            {
                OutErrorMessage = FString::Printf(TEXT("Property not found: %s"), *ParsedSegment.Name);
                return false;
            }

            void* PropertyAddress = Property->ContainerPtrToValuePtr<void>(CurrentContainer);
            const bool bIsLastSegment = SegmentIndex == Segments.Num() - 1;

            if (!ParsedSegment.bHasArrayIndex)
            {
                if (bIsLastSegment)
                {
                    OutProperty = Property;
                    OutPropertyAddress = PropertyAddress;
                    return true;
                }

                if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
                {
                    CurrentContainer = PropertyAddress;
                    CurrentStruct = StructProperty->Struct;
                    continue;
                }

                if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
                {
                    UObject* NestedObject = ObjectProperty->GetObjectPropertyValue(PropertyAddress);
                    if (!NestedObject)
                    {
                        OutErrorMessage = FString::Printf(TEXT("Cannot resolve nested property path '%s': '%s' is null"), *PropertyPath, *Property->GetName());
                        return false;
                    }

                    CurrentContainer = NestedObject;
                    CurrentStruct = NestedObject->GetClass();
                    continue;
                }

                OutErrorMessage = FString::Printf(
                    TEXT("Cannot traverse nested property path '%s' through '%s'"),
                    *PropertyPath,
                    *Property->GetName());
                return false;
            }

            FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property);
            if (!ArrayProperty)
            {
                OutErrorMessage = FString::Printf(TEXT("Property '%s' is not an array but index was requested"), *Property->GetName());
                return false;
            }

            FScriptArrayHelper ArrayHelper(ArrayProperty, PropertyAddress);
            if (!ArrayHelper.IsValidIndex(ParsedSegment.ArrayIndex))
            {
                OutErrorMessage = FString::Printf(
                    TEXT("Array index %d is out of range for '%s' (size: %d)"),
                    ParsedSegment.ArrayIndex,
                    *Property->GetName(),
                    ArrayHelper.Num());
                return false;
            }

            void* ElementAddress = ArrayHelper.GetRawPtr(ParsedSegment.ArrayIndex);
            if (bIsLastSegment)
            {
                OutProperty = ArrayProperty->Inner;
                OutPropertyAddress = ElementAddress;
                return true;
            }

            if (FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProperty->Inner))
            {
                CurrentContainer = ElementAddress;
                CurrentStruct = InnerStruct->Struct;
                continue;
            }

            if (FObjectPropertyBase* InnerObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner))
            {
                UObject* NestedObject = InnerObjectProperty->GetObjectPropertyValue(ElementAddress);
                if (!NestedObject)
                {
                    OutErrorMessage = FString::Printf(TEXT("Array element '%s[%d]' is null and cannot be traversed"), *Property->GetName(), ParsedSegment.ArrayIndex);
                    return false;
                }

                CurrentContainer = NestedObject;
                CurrentStruct = NestedObject->GetClass();
                continue;
            }

            OutErrorMessage = FString::Printf(
                TEXT("Cannot traverse nested property path '%s' through array element type '%s'"),
                *PropertyPath,
                *ArrayProperty->Inner->GetClass()->GetName());
            return false;
        }

        OutErrorMessage = FString::Printf(TEXT("Property not found: %s"), *PropertyPath);
        return false;
    }

    bool SetEnumPropertyValue(FProperty* Property, void* PropertyAddress, const TSharedPtr<FJsonValue>& JsonValue, FString& OutErrorMessage)
    {
        UEnum* EnumDefinition = nullptr;
        FNumericProperty* NumericProperty = nullptr;

        if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
        {
            EnumDefinition = EnumProperty->GetEnum();
            NumericProperty = EnumProperty->GetUnderlyingProperty();
        }
        else if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
        {
            EnumDefinition = ByteProperty->GetIntPropertyEnum();
            NumericProperty = ByteProperty;
        }

        if (!EnumDefinition || !NumericProperty)
        {
            OutErrorMessage = FString::Printf(TEXT("Property '%s' is not an enum"), *Property->GetName());
            return false;
        }

        int64 EnumValue = INDEX_NONE;
        double NumericValue = 0.0;

        if (TryGetJsonNumber(JsonValue, NumericValue))
        {
            EnumValue = static_cast<int64>(NumericValue);
        }
        else if (JsonValue.IsValid() && JsonValue->Type == EJson::String)
        {
            FString EnumValueName = JsonValue->AsString().TrimStartAndEnd();
            if (EnumValueName.Contains(TEXT("::")))
            {
                EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
            }

            EnumValue = EnumDefinition->GetValueByNameString(EnumValueName, EGetByNameFlags::CaseSensitive);
            if (EnumValue == INDEX_NONE)
            {
                EnumValue = EnumDefinition->GetValueByNameString(EnumValueName, EGetByNameFlags::None);
            }
        }

        if (EnumValue == INDEX_NONE)
        {
            OutErrorMessage = FString::Printf(
                TEXT("Could not map value '%s' to enum '%s'"),
                JsonValue.IsValid() ? *JsonValue->AsString() : TEXT("<null>"),
                *EnumDefinition->GetName());
            return false;
        }

        NumericProperty->SetIntPropertyValue(PropertyAddress, EnumValue);
        return true;
    }

    bool SetStructPropertyValue(FStructProperty* StructProperty, void* PropertyAddress, const TSharedPtr<FJsonValue>& JsonValue, FString& OutErrorMessage)
    {
        if (!StructProperty || !PropertyAddress)
        {
            OutErrorMessage = TEXT("Invalid struct property");
            return false;
        }

        if (!JsonValue.IsValid() || JsonValue->IsNull())
        {
            OutErrorMessage = FString::Printf(TEXT("Cannot assign null to struct property '%s'"), *StructProperty->GetName());
            return false;
        }

        UScriptStruct* ScriptStruct = StructProperty->Struct;
        if (JsonValue->Type == EJson::String)
        {
            return StructProperty->ImportText_Direct(*JsonValue->AsString(), PropertyAddress, nullptr, PPF_None) != nullptr;
        }

        if (ScriptStruct == TBaseStructure<FVector>::Get())
        {
            FVector ParsedValue(0.0f);
            if (JsonValue->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Values = JsonValue->AsArray();
                if (Values.Num() >= 3)
                {
                    ParsedValue.X = static_cast<float>(Values[0]->AsNumber());
                    ParsedValue.Y = static_cast<float>(Values[1]->AsNumber());
                    ParsedValue.Z = static_cast<float>(Values[2]->AsNumber());
                    *reinterpret_cast<FVector*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }
            else if (JsonValue->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject>& ObjectValue = JsonValue->AsObject();
                double X = 0.0;
                double Y = 0.0;
                double Z = 0.0;
                if (TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("x"), X) &&
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("y"), Y) &&
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("z"), Z))
                {
                    ParsedValue.X = static_cast<float>(X);
                    ParsedValue.Y = static_cast<float>(Y);
                    ParsedValue.Z = static_cast<float>(Z);
                    *reinterpret_cast<FVector*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }

            OutErrorMessage = FString::Printf(TEXT("Failed to parse FVector for '%s'"), *StructProperty->GetName());
            return false;
        }

        if (ScriptStruct == TBaseStructure<FVector2D>::Get())
        {
            FVector2D ParsedValue(0.0f);
            if (JsonValue->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Values = JsonValue->AsArray();
                if (Values.Num() >= 2)
                {
                    ParsedValue.X = static_cast<float>(Values[0]->AsNumber());
                    ParsedValue.Y = static_cast<float>(Values[1]->AsNumber());
                    *reinterpret_cast<FVector2D*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }
            else if (JsonValue->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject>& ObjectValue = JsonValue->AsObject();
                double X = 0.0;
                double Y = 0.0;
                if (TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("x"), X) &&
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("y"), Y))
                {
                    ParsedValue.X = static_cast<float>(X);
                    ParsedValue.Y = static_cast<float>(Y);
                    *reinterpret_cast<FVector2D*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }

            OutErrorMessage = FString::Printf(TEXT("Failed to parse FVector2D for '%s'"), *StructProperty->GetName());
            return false;
        }

        if (ScriptStruct == TBaseStructure<FRotator>::Get())
        {
            FRotator ParsedValue(0.0f);
            if (JsonValue->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Values = JsonValue->AsArray();
                if (Values.Num() >= 3)
                {
                    ParsedValue.Pitch = static_cast<float>(Values[0]->AsNumber());
                    ParsedValue.Yaw = static_cast<float>(Values[1]->AsNumber());
                    ParsedValue.Roll = static_cast<float>(Values[2]->AsNumber());
                    *reinterpret_cast<FRotator*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }
            else if (JsonValue->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject>& ObjectValue = JsonValue->AsObject();
                double Pitch = 0.0;
                double Yaw = 0.0;
                double Roll = 0.0;
                if (TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("pitch"), Pitch) &&
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("yaw"), Yaw) &&
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("roll"), Roll))
                {
                    ParsedValue.Pitch = static_cast<float>(Pitch);
                    ParsedValue.Yaw = static_cast<float>(Yaw);
                    ParsedValue.Roll = static_cast<float>(Roll);
                    *reinterpret_cast<FRotator*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }

            OutErrorMessage = FString::Printf(TEXT("Failed to parse FRotator for '%s'"), *StructProperty->GetName());
            return false;
        }

        if (ScriptStruct == TBaseStructure<FLinearColor>::Get())
        {
            FLinearColor ParsedValue(0.0f, 0.0f, 0.0f, 1.0f);
            if (JsonValue->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Values = JsonValue->AsArray();
                if (Values.Num() >= 3)
                {
                    ParsedValue.R = static_cast<float>(Values[0]->AsNumber());
                    ParsedValue.G = static_cast<float>(Values[1]->AsNumber());
                    ParsedValue.B = static_cast<float>(Values[2]->AsNumber());
                    ParsedValue.A = Values.Num() >= 4 ? static_cast<float>(Values[3]->AsNumber()) : 1.0f;
                    *reinterpret_cast<FLinearColor*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }
            else if (JsonValue->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject>& ObjectValue = JsonValue->AsObject();
                double R = 0.0;
                double G = 0.0;
                double B = 0.0;
                double A = 1.0;
                if (TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("r"), R) &&
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("g"), G) &&
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("b"), B))
                {
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("a"), A);
                    ParsedValue.R = static_cast<float>(R);
                    ParsedValue.G = static_cast<float>(G);
                    ParsedValue.B = static_cast<float>(B);
                    ParsedValue.A = static_cast<float>(A);
                    *reinterpret_cast<FLinearColor*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }

            OutErrorMessage = FString::Printf(TEXT("Failed to parse FLinearColor for '%s'"), *StructProperty->GetName());
            return false;
        }

        if (ScriptStruct == TBaseStructure<FColor>::Get())
        {
            FColor ParsedValue(0, 0, 0, 255);
            if (JsonValue->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Values = JsonValue->AsArray();
                if (Values.Num() >= 3)
                {
                    ParsedValue.R = static_cast<uint8>(FMath::Clamp(static_cast<int32>(Values[0]->AsNumber()), 0, 255));
                    ParsedValue.G = static_cast<uint8>(FMath::Clamp(static_cast<int32>(Values[1]->AsNumber()), 0, 255));
                    ParsedValue.B = static_cast<uint8>(FMath::Clamp(static_cast<int32>(Values[2]->AsNumber()), 0, 255));
                    ParsedValue.A = Values.Num() >= 4 ? static_cast<uint8>(FMath::Clamp(static_cast<int32>(Values[3]->AsNumber()), 0, 255)) : 255;
                    *reinterpret_cast<FColor*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }
            else if (JsonValue->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject>& ObjectValue = JsonValue->AsObject();
                double R = 0.0;
                double G = 0.0;
                double B = 0.0;
                double A = 255.0;
                if (TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("r"), R) &&
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("g"), G) &&
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("b"), B))
                {
                    TryGetNumberFieldCaseInsensitive(ObjectValue, TEXT("a"), A);
                    ParsedValue.R = static_cast<uint8>(FMath::Clamp(static_cast<int32>(R), 0, 255));
                    ParsedValue.G = static_cast<uint8>(FMath::Clamp(static_cast<int32>(G), 0, 255));
                    ParsedValue.B = static_cast<uint8>(FMath::Clamp(static_cast<int32>(B), 0, 255));
                    ParsedValue.A = static_cast<uint8>(FMath::Clamp(static_cast<int32>(A), 0, 255));
                    *reinterpret_cast<FColor*>(PropertyAddress) = ParsedValue;
                    return true;
                }
            }

            OutErrorMessage = FString::Printf(TEXT("Failed to parse FColor for '%s'"), *StructProperty->GetName());
            return false;
        }

        OutErrorMessage = FString::Printf(
            TEXT("Unsupported struct assignment for '%s' (%s). Provide Unreal text format for this struct."),
            *StructProperty->GetName(),
            *ScriptStruct->GetName());
        return false;
    }

    bool SetPropertyValueFromJson(FProperty* Property, void* PropertyAddress, const TSharedPtr<FJsonValue>& JsonValue, FString& OutErrorMessage)
    {
        if (!Property || !PropertyAddress)
        {
            OutErrorMessage = TEXT("Invalid property destination");
            return false;
        }

        if (!JsonValue.IsValid())
        {
            OutErrorMessage = FString::Printf(TEXT("Invalid JSON value for property '%s'"), *Property->GetName());
            return false;
        }

        if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
        {
            bool ParsedBool = false;
            if (!TryGetJsonBool(JsonValue, ParsedBool))
            {
                OutErrorMessage = FString::Printf(TEXT("Expected boolean-compatible value for '%s'"), *Property->GetName());
                return false;
            }

            BoolProperty->SetPropertyValue(PropertyAddress, ParsedBool);
            return true;
        }

        if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
        {
            if (NumericProperty->IsEnum())
            {
                return SetEnumPropertyValue(Property, PropertyAddress, JsonValue, OutErrorMessage);
            }

            double ParsedNumber = 0.0;
            if (!TryGetJsonNumber(JsonValue, ParsedNumber))
            {
                OutErrorMessage = FString::Printf(TEXT("Expected numeric value for '%s'"), *Property->GetName());
                return false;
            }

            if (NumericProperty->IsFloatingPoint())
            {
                NumericProperty->SetFloatingPointPropertyValue(PropertyAddress, ParsedNumber);
            }
            else
            {
                NumericProperty->SetIntPropertyValue(PropertyAddress, static_cast<int64>(ParsedNumber));
            }

            return true;
        }

        if (FStrProperty* StringProperty = CastField<FStrProperty>(Property))
        {
            FString ParsedString;
            if (!TryGetStringLikeValue(JsonValue, ParsedString))
            {
                OutErrorMessage = FString::Printf(TEXT("Expected string-compatible value for '%s'"), *Property->GetName());
                return false;
            }

            StringProperty->SetPropertyValue(PropertyAddress, ParsedString);
            return true;
        }

        if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
        {
            FString ParsedName;
            if (!TryGetStringLikeValue(JsonValue, ParsedName))
            {
                OutErrorMessage = FString::Printf(TEXT("Expected name-compatible value for '%s'"), *Property->GetName());
                return false;
            }

            NameProperty->SetPropertyValue(PropertyAddress, FName(*ParsedName));
            return true;
        }

        if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
        {
            FString ParsedText;
            if (!TryGetStringLikeValue(JsonValue, ParsedText))
            {
                OutErrorMessage = FString::Printf(TEXT("Expected text-compatible value for '%s'"), *Property->GetName());
                return false;
            }

            TextProperty->SetPropertyValue(PropertyAddress, FText::FromString(ParsedText));
            return true;
        }

        if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
        {
            return SetEnumPropertyValue(EnumProperty, PropertyAddress, JsonValue, OutErrorMessage);
        }

        if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
        {
            if (IsJsonNullLike(JsonValue))
            {
                ObjectProperty->SetObjectPropertyValue(PropertyAddress, nullptr);
                return true;
            }

            FString AssetPath;
            if (JsonValue->Type == EJson::String)
            {
                AssetPath = JsonValue->AsString();
            }
            else if (JsonValue->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject>& ObjectValue = JsonValue->AsObject();
                if (!ObjectValue->TryGetStringField(TEXT("path"), AssetPath) &&
                    !ObjectValue->TryGetStringField(TEXT("asset_path"), AssetPath))
                {
                    OutErrorMessage = FString::Printf(TEXT("Expected object with 'path' for object property '%s'"), *Property->GetName());
                    return false;
                }
            }
            else
            {
                OutErrorMessage = FString::Printf(TEXT("Unsupported value type for object property '%s'"), *Property->GetName());
                return false;
            }

            UObject* LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
            if (!LoadedObject)
            {
                OutErrorMessage = FString::Printf(TEXT("Failed to load object from '%s'"), *AssetPath);
                return false;
            }

            if (!LoadedObject->IsA(ObjectProperty->PropertyClass))
            {
                OutErrorMessage = FString::Printf(
                    TEXT("Object '%s' is not of expected type '%s' for property '%s'"),
                    *LoadedObject->GetClass()->GetName(),
                    *ObjectProperty->PropertyClass->GetName(),
                    *Property->GetName());
                return false;
            }

            ObjectProperty->SetObjectPropertyValue(PropertyAddress, LoadedObject);
            return true;
        }

        if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
        {
            FString SoftPathText = TEXT("None");
            if (!IsJsonNullLike(JsonValue))
            {
                if (!TryGetStringLikeValue(JsonValue, SoftPathText))
                {
                    OutErrorMessage = FString::Printf(TEXT("Expected soft object path string for '%s'"), *Property->GetName());
                    return false;
                }
            }
            if (SoftObjectProperty->ImportText_Direct(*SoftPathText, PropertyAddress, nullptr, PPF_None) != nullptr)
            {
                return true;
            }

            OutErrorMessage = FString::Printf(TEXT("Failed to assign soft object path '%s' to '%s'"), *SoftPathText, *Property->GetName());
            return false;
        }

        if (FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
        {
            FString SoftClassText = TEXT("None");
            if (!IsJsonNullLike(JsonValue))
            {
                if (!TryGetStringLikeValue(JsonValue, SoftClassText))
                {
                    OutErrorMessage = FString::Printf(TEXT("Expected soft class path string for '%s'"), *Property->GetName());
                    return false;
                }
            }
            if (SoftClassProperty->ImportText_Direct(*SoftClassText, PropertyAddress, nullptr, PPF_None) != nullptr)
            {
                return true;
            }

            OutErrorMessage = FString::Printf(TEXT("Failed to assign soft class path '%s' to '%s'"), *SoftClassText, *Property->GetName());
            return false;
        }

        if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
        {
            if (IsJsonNullLike(JsonValue))
            {
                ClassProperty->SetPropertyValue(PropertyAddress, nullptr);
                return true;
            }

            FString ClassPath;
            if (!TryGetStringLikeValue(JsonValue, ClassPath))
            {
                OutErrorMessage = FString::Printf(TEXT("Expected class path string for '%s'"), *Property->GetName());
                return false;
            }

            UClass* ExpectedClass = ClassProperty->MetaClass ? ClassProperty->MetaClass.Get() : UObject::StaticClass();
            UClass* LoadedClass = StaticLoadClass(ExpectedClass, nullptr, *ClassPath);
            if (!LoadedClass)
            {
                OutErrorMessage = FString::Printf(TEXT("Failed to load class from '%s'"), *ClassPath);
                return false;
            }

            if (ClassProperty->MetaClass && !LoadedClass->IsChildOf(ClassProperty->MetaClass.Get()))
            {
                OutErrorMessage = FString::Printf(
                    TEXT("Class '%s' is not a child of '%s' for property '%s'"),
                    *LoadedClass->GetName(),
                    *ClassProperty->MetaClass->GetName(),
                    *Property->GetName());
                return false;
            }

            ClassProperty->SetPropertyValue(PropertyAddress, LoadedClass);
            return true;
        }

        if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
        {
            return SetStructPropertyValue(StructProperty, PropertyAddress, JsonValue, OutErrorMessage);
        }

        if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
        {
            if (JsonValue->Type != EJson::Array)
            {
                OutErrorMessage = FString::Printf(TEXT("Expected array value for set property '%s'"), *Property->GetName());
                return false;
            }

            FScriptSetHelper SetHelper(SetProperty, PropertyAddress);
            SetHelper.EmptyElements();
            const TArray<TSharedPtr<FJsonValue>>& SourceArray = JsonValue->AsArray();
            for (const TSharedPtr<FJsonValue>& EntryValue : SourceArray)
            {
                const int32 NewIndex = SetHelper.AddDefaultValue_Invalid_NeedsRehash();
                void* ElementAddress = SetHelper.GetElementPtr(NewIndex);
                FString EntryError;
                if (!SetPropertyValueFromJson(SetProperty->ElementProp, ElementAddress, EntryValue, EntryError))
                {
                    OutErrorMessage = FString::Printf(TEXT("Failed to set set entry for '%s': %s"), *Property->GetName(), *EntryError);
                    return false;
                }
            }

            SetHelper.Rehash();
            return true;
        }

        if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
        {
            FScriptMapHelper MapHelper(MapProperty, PropertyAddress);
            MapHelper.EmptyValues();

            auto AddMapEntry = [&](const TSharedPtr<FJsonValue>& KeyValue, const TSharedPtr<FJsonValue>& ValueValue, FString& OutEntryError) -> bool
            {
                const int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
                uint8* PairPtr = MapHelper.GetPairPtr(NewIndex);
                void* KeyAddress = MapProperty->KeyProp->ContainerPtrToValuePtr<void>(PairPtr);
                void* ValueAddress = MapProperty->ValueProp->ContainerPtrToValuePtr<void>(PairPtr);

                if (!SetPropertyValueFromJson(MapProperty->KeyProp, KeyAddress, KeyValue, OutEntryError))
                {
                    return false;
                }

                if (!SetPropertyValueFromJson(MapProperty->ValueProp, ValueAddress, ValueValue, OutEntryError))
                {
                    return false;
                }

                return true;
            };

            if (JsonValue->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject>& SourceObject = JsonValue->AsObject();
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : SourceObject->Values)
                {
                    FString EntryError;
                    if (!AddMapEntry(MakeShared<FJsonValueString>(Field.Key), Field.Value, EntryError))
                    {
                        OutErrorMessage = FString::Printf(TEXT("Failed to add map entry for '%s': %s"), *Property->GetName(), *EntryError);
                        return false;
                    }
                }

                MapHelper.Rehash();
                return true;
            }

            if (JsonValue->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& SourceEntries = JsonValue->AsArray();
                for (int32 EntryIndex = 0; EntryIndex < SourceEntries.Num(); ++EntryIndex)
                {
                    const TSharedPtr<FJsonValue>& Entry = SourceEntries[EntryIndex];
                    if (!Entry.IsValid() || Entry->Type != EJson::Object)
                    {
                        OutErrorMessage = FString::Printf(TEXT("Map entry %d for '%s' must be an object with key/value fields"), EntryIndex, *Property->GetName());
                        return false;
                    }

                    const TSharedPtr<FJsonObject>& EntryObject = Entry->AsObject();
                    TSharedPtr<FJsonValue> KeyValue = EntryObject->Values.FindRef(TEXT("key"));
                    TSharedPtr<FJsonValue> ValueValue = EntryObject->Values.FindRef(TEXT("value"));
                    if (!KeyValue.IsValid() || !ValueValue.IsValid())
                    {
                        OutErrorMessage = FString::Printf(TEXT("Map entry %d for '%s' is missing 'key' or 'value'"), EntryIndex, *Property->GetName());
                        return false;
                    }

                    FString EntryError;
                    if (!AddMapEntry(KeyValue, ValueValue, EntryError))
                    {
                        OutErrorMessage = FString::Printf(TEXT("Failed to add map entry %d for '%s': %s"), EntryIndex, *Property->GetName(), *EntryError);
                        return false;
                    }
                }

                MapHelper.Rehash();
                return true;
            }

            OutErrorMessage = FString::Printf(TEXT("Expected object or array value for map property '%s'"), *Property->GetName());
            return false;
        }

        if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
        {
            if (JsonValue->Type != EJson::Array)
            {
                OutErrorMessage = FString::Printf(TEXT("Expected array value for property '%s'"), *Property->GetName());
                return false;
            }

            const TArray<TSharedPtr<FJsonValue>>& SourceArray = JsonValue->AsArray();
            FScriptArrayHelper ArrayHelper(ArrayProperty, PropertyAddress);
            ArrayHelper.Resize(SourceArray.Num());

            for (int32 Index = 0; Index < SourceArray.Num(); ++Index)
            {
                FString ElementError;
                if (!SetPropertyValueFromJson(ArrayProperty->Inner, ArrayHelper.GetRawPtr(Index), SourceArray[Index], ElementError))
                {
                    OutErrorMessage = FString::Printf(
                        TEXT("Failed to set array element %d for '%s': %s"),
                        Index,
                        *Property->GetName(),
                        *ElementError);
                    return false;
                }
            }

            return true;
        }

        if (JsonValue->Type == EJson::String)
        {
            if (Property->ImportText_Direct(*JsonValue->AsString(), PropertyAddress, nullptr, PPF_None) != nullptr)
            {
                return true;
            }
        }

        OutErrorMessage = FString::Printf(
            TEXT("Unsupported property type '%s' for '%s' with JSON type '%s'"),
            *Property->GetClass()->GetName(),
            *Property->GetName(),
            *JsonTypeToString(JsonValue));
        return false;
    }
}
