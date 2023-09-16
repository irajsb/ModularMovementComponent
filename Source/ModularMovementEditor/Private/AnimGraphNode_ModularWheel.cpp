//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "AnimGraphNode_ModularWheel.h"

#include "ModularVehicleAnimationInstance.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_ModularWheel

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_ModularWheel::UAnimGraphNode_ModularWheel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_ModularWheel::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNode_ModularWheel", "Wheel Controller for Modular Vehicle");
}

FText UAnimGraphNode_ModularWheel::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_ModularWheel_Tooltip", "This alters the wheel transform based on set up in Modular Vehicle. This only works when the owner is WheeledVehicle.");
}

FText UAnimGraphNode_ModularWheel::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText NodeTitle;
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		NodeTitle = GetControllerDescription();
	}
	else
	{
		// we don't have any run-time information, so it's limited to print  
		// anymore than what it is it would be nice to print more data such as 
		// name of bones for wheels, but it's not available in Persona
		NodeTitle = FText(LOCTEXT("AnimGraphNode_ModularWheel_Title", "Modular Wheel Controller"));
	}	
	return NodeTitle;
}




void UAnimGraphNode_ModularWheel::ValidateAnimNodePostCompile(class FCompilerResultsLog& MessageLog, class UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex)
{
	// we only support vehicle anim instance
	if ( CompiledClass->IsChildOf(UModularVehicleAnimationInstance::StaticClass())  == false )
	{
		MessageLog.Error(TEXT("@@ is only allowwed in ModularVehicleAnimInstance. If this is for vehicle, please change parent to be VehicleAnimInstance (Reparent Class)."), this);
	}
}

bool UAnimGraphNode_ModularWheel::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return (Blueprint != nullptr) && Blueprint->ParentClass->IsChildOf<UModularVehicleAnimationInstance>() && Super::IsCompatibleWithGraph(TargetGraph);
}

#undef LOCTEXT_NAMESPACE
