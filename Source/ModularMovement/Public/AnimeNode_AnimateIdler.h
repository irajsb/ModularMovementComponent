//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"

#include "BoneControllers/AnimNode_SkeletalControlBase.h"

#include "AnimeNode_AnimateIdler.generated.h"

class UTankTrackComponent;
/**
 *	Simple controller that replaces or adds to the translation/rotation of a single bone.
 */
USTRUCT()
struct MODULARMOVEMENT_API FAnimNode_AnimateIdler : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Name of bone to control. This is the main bone chain to modify from. **/
	UPROPERTY(EditAnywhere, Category=Data) 
	FBoneReference TargetBone;
	UPROPERTY(EditAnywhere, Category=Data,meta = (PinShownByDefault))
	UTankTrackComponent* TrackComponent;
	UPROPERTY(EditAnywhere, Category=Data,meta = (PinShownByDefault))
	float Radius=30;
	 
	UPROPERTY(EditAnywhere, Category=Data,meta = (PinShownByDefault))
	int OptionalTeethCount=-1;

	
	FAnimNode_AnimateIdler();
	float LastLenDiff;
	float AnimRot;
	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
virtual void UpdateComponentPose_AnyThread(const FAnimationUpdateContext& Context) override;
	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	
	// End of FAnimNode_SkeletalControlBase interface

	private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	



};
