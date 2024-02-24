//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "AnimGraphNode_AnimateIdler.h"

#include "ModularVehicleAnimationInstance.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_AnimateIdler

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_AnimateIdler::UAnimGraphNode_AnimateIdler(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_AnimateIdler::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNode_AnimateIdler", "Copy Rotation From Track");
}

FText UAnimGraphNode_AnimateIdler::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_ModularWheel_Tooltip", "Animate non physical wheels from Track ");
}

FText UAnimGraphNode_AnimateIdler::GetNodeTitle(ENodeTitleType::Type TitleType) const
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
		NodeTitle = FText(LOCTEXT("AnimGraphNode_AnimateIdler_Title", "Copy Rotation From Track"));
	}	
	return NodeTitle;
}




void UAnimGraphNode_AnimateIdler::ValidateAnimNodePostCompile(class FCompilerResultsLog& MessageLog, class UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex)
{
	// we only support vehicle anim instance
	if ( CompiledClass->IsChildOf(UModularVehicleAnimationInstance::StaticClass())  == false )
	{
		MessageLog.Error(TEXT("@@ is only allowwed in ModularVehicleAnimInstance. If this is for vehicle, please change parent to be VehicleAnimInstance (Reparent Class)."), this);
	}
}

bool UAnimGraphNode_AnimateIdler::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return (Blueprint != nullptr) && Blueprint->ParentClass->IsChildOf<UModularVehicleAnimationInstance>() && Super::IsCompatibleWithGraph(TargetGraph);
}

#undef LOCTEXT_NAMESPACE
