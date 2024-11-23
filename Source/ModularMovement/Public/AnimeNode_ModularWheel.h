//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "MMVehicleAnimationInstance.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"

#include "AnimeNode_ModularWheel.generated.h"

/**
 *	Simple controller that replaces or adds to the translation/rotation of a single bone.
 */
USTRUCT()
struct MODULARMOVEMENT_API FAnimNode_ModularWheel : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vehicle, meta = (PinShownByDefault))
	FString WheelPrefix;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vehicle, meta = (PinShownByDefault))
	FString SuspPrefix;
	virtual  ~FAnimNode_ModularWheel() override;
	FAnimNode_ModularWheel();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	// End of FAnimNode_SkeletalControlBase interface

	private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	struct FWheelLookupData
	{
		int32 WheelIndex;
		FBoneReference WheelReference;
		bool IsSusp=false;
		
	};

	TArray<FWheelLookupData> Wheels;
	const FMMVehicleAnimationInstanceProxy* AnimInstanceProxy;	//TODO: we only cache this to use in eval where it's safe. Should change API to pass proxy into eval
};
