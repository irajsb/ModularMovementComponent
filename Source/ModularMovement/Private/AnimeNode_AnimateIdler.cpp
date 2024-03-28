//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "AnimeNode_AnimateIdler.h"

#include "TankTrackComponent.h"
#include "Animation/AnimTrace.h"


FAnimNode_AnimateIdler::FAnimNode_AnimateIdler(): TrackComponent(nullptr), AnimRot(0)
{
}


void FAnimNode_AnimateIdler::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);
	
	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT("  Dst: %s)"), *TargetBone.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_AnimateIdler::UpdateComponentPose_AnyThread(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateComponentPose_AnyThread(Context);

	if(!TrackComponent)
	{
		return;
	}


	if(OptionalTeethCount<0)
	{
		AnimRot += TrackComponent->TrackSpeed/Radius*	Context.GetDeltaTime();


		float IntegerPart = 0.f;
		AnimRot= FMath::Modf(AnimRot / (2 * PI), &IntegerPart) * (2 * PI);
	}else
	{
		const int TrackCount=TrackComponent->NumOfMeshesInTrack;
		//to angular
		float AngularSpeed=TrackComponent->TrackSpeed/(TrackComponent->GetSplineLength()/(2*PI));
		AngularSpeed=AngularSpeed*TrackCount/OptionalTeethCount;
		//Teeth per second
		AnimRot += AngularSpeed*	Context.GetDeltaTime();
		float IntegerPart = 0.f;
		AnimRot= FMath::Modf(AnimRot / (2 * PI), &IntegerPart) * (2 * PI);
	
	}
	
}

void FAnimNode_AnimateIdler::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
                                                               TArray<FBoneTransform>& OutBoneTransforms)
{
	if(!TrackComponent)
	{
		return;
	}




	check(OutBoneTransforms.Num() == 0);



	// Get component space transform for source and current bone.
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	
	FCompactPoseBoneIndex TargetBoneIndex = TargetBone.GetCompactPoseIndex(BoneContainer);

	
	FTransform CurrentBoneTM = Output.Pose.GetComponentSpaceTransform(TargetBoneIndex);

	
		
		FRotator Rotation = FRotator(FMath::RadiansToDegrees(-1 * AnimRot), 0, 0);
	
	
		CurrentBoneTM.SetRotation(Rotation.Quaternion() );
	

	

	// Output new transform for current bone.
	OutBoneTransforms.Add(FBoneTransform(TargetBoneIndex, CurrentBoneTM));


	TRACE_ANIM_NODE_VALUE(Output, TEXT("Target Bone"), TargetBone.BoneName);
}

bool FAnimNode_AnimateIdler::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	
	
		return  (TargetBone.IsValidToEvaluate()&&TrackComponent);
	
	
}

void FAnimNode_AnimateIdler::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{

	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(InitializeBoneReferences)

	TargetBone.Initialize(RequiredBones);
	
}


