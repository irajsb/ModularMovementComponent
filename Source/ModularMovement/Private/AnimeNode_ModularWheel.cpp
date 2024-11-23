//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "AnimeNode_ModularWheel.h"


FAnimNode_ModularWheel::~FAnimNode_ModularWheel()=default;


FAnimNode_ModularWheel::FAnimNode_ModularWheel()
{
	AnimInstanceProxy = nullptr;
}

void FAnimNode_ModularWheel::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += ")";

	DebugData.AddDebugItem(DebugLine);

	const TArray<FModularWheelAnimationData>& WheelAnimData = AnimInstanceProxy->GetWheelAnimData();
	for (const FWheelLookupData& Wheel : Wheels)
	{
		if (Wheel.WheelReference.BoneIndex != INDEX_NONE)
		{
			DebugLine = FString::Printf(
				TEXT(" [Wheel Index : %d] Bone: %s , Rotation Offset : %s, Location Offset : %s"),
				Wheel.WheelIndex, *Wheel.WheelReference.BoneName.ToString(),
				*WheelAnimData[Wheel.WheelIndex].RotOffset.ToString(),
				*WheelAnimData[Wheel.WheelIndex].LocOffset.ToString());
		}
		else
		{
			DebugLine = FString::Printf(TEXT(" [Wheel Index : %d] Bone: %s (invalid bone)"),
			                            Wheel.WheelIndex, *Wheel.WheelReference.BoneName.ToString());
		}

		DebugData.AddDebugItem(DebugLine);
	}

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_ModularWheel::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
                                                               TArray<FBoneTransform>& OutBoneTransforms)
{
	const TArray<FModularWheelAnimationData>& WheelAnimData = AnimInstanceProxy->GetWheelAnimData();

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	for (const FWheelLookupData& Wheel : Wheels)
	{
		if (Wheel.WheelReference.IsValidToEvaluate(BoneContainer))
		{
			const FCompactPoseBoneIndex WheelSimBoneIndex = Wheel.WheelReference.GetCompactPoseIndex(BoneContainer);

			// the way we apply transform is same as FMatrix or FTransform
			// we apply scale first, and rotation, and translation
			// if you'd like to translate first, you'll need two nodes that first node does translate and second nodes to rotate. 
			FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(WheelSimBoneIndex);

			FAnimationRuntime::ConvertCSTransformToBoneSpace(Output.AnimInstanceProxy->GetComponentTransform(),
			                                                 Output.Pose, NewBoneTM, WheelSimBoneIndex,
			                                                 BCS_ComponentSpace);

			if (!Wheel.IsSusp)
			{
				// Apply rotation offset
				const FQuat BoneQuat(WheelAnimData[Wheel.WheelIndex].RotOffset);
				NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());
			}
			// Apply loc offset
			NewBoneTM.AddToTranslation(WheelAnimData[Wheel.WheelIndex].LocOffset);

			// Convert back to Component Space.
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(Output.AnimInstanceProxy->GetComponentTransform(),
			                                                 Output.Pose, NewBoneTM, WheelSimBoneIndex,
			                                                 BCS_ComponentSpace);

			// add back to it
			OutBoneTransforms.Add(FBoneTransform(WheelSimBoneIndex, NewBoneTM));
		}
	}

#if ANIM_TRACE_ENABLED
	for (const FWheelLookupData& Wheel : Wheels)
	{
		if (Wheel.WheelReference.BoneIndex != INDEX_NONE)
		{
			TRACE_ANIM_NODE_VALUE(Output, *FString::Printf(TEXT("Wheel %d Name"), Wheel.WheelIndex),
			                      *Wheel.WheelReference.BoneName.ToString());
			TRACE_ANIM_NODE_VALUE(Output, *FString::Printf(TEXT("Wheel %d Rotation Offset"), Wheel.WheelIndex),
			                      WheelAnimData[Wheel.WheelIndex].RotOffset);
			TRACE_ANIM_NODE_VALUE(Output, *FString::Printf(TEXT("Wheel %d Location Offset"), Wheel.WheelIndex),
			                      WheelAnimData[Wheel.WheelIndex].LocOffset);
		}
		else
		{
			TRACE_ANIM_NODE_VALUE(Output, *FString::Printf(TEXT("Wheel %d Name"), Wheel.WheelIndex),
			                      *FString::Printf(TEXT("%s (invalid)"), *Wheel.WheelReference.BoneName.ToString()));
		}
	}
#endif
}

bool FAnimNode_ModularWheel::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	// if both bones are valid
	for (const FWheelLookupData& Wheel : Wheels)
	{
		// if one of them is valid
		if (Wheel.WheelReference.IsValidToEvaluate(RequiredBones) == true)
		{
			return true;
		}
	}

	return false;
}

void FAnimNode_ModularWheel::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	const TArray<FModularWheelAnimationData>& WheelAnimData = AnimInstanceProxy->GetWheelAnimData();
	const int32 NumWheels = WheelAnimData.Num();
	Wheels.Empty(NumWheels);

	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		FWheelLookupData* Wheel = new(Wheels)FWheelLookupData();
		Wheel->WheelIndex = WheelIndex;
		Wheel->WheelReference.BoneName = FName(WheelPrefix + WheelAnimData[WheelIndex].BoneName.ToString());
		Wheel->WheelReference.Initialize(RequiredBones);
		if (!SuspPrefix.IsEmpty())
		{
			FWheelLookupData* Suspension = new(Wheels)FWheelLookupData();
			Suspension->WheelIndex = WheelIndex;
			Suspension->WheelReference.BoneName = FName(SuspPrefix + WheelAnimData[WheelIndex].BoneName.ToString());
			Suspension->WheelReference.Initialize(RequiredBones);
			Suspension->IsSusp = true;
		}
	}

	// sort by bone indices
	Wheels.Sort([](const FWheelLookupData& L, const FWheelLookupData& R)
	{
		return L.WheelReference.BoneIndex < R.WheelReference.BoneIndex;
	});
}

void FAnimNode_ModularWheel::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	AnimInstanceProxy = static_cast<FMMVehicleAnimationInstanceProxy*>(Context.AnimInstanceProxy);
	
}
