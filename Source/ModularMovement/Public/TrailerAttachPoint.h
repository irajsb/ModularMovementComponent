// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "TrailerAttachPoint.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTrailerPromptShow,bool,NewVisibility,bool,ShouldShowDeattachPrompt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTrailerAttach,bool,NewAttach,bool,FromSave,UPhysicsConstraintComponent* ,ConstraintComponent);
/**
 * 
 */
UCLASS()
class MODULARMOVEMENT_API UTrailerAttachPoint : public UStaticMeshComponent
{
	GENERATED_BODY()

	virtual void BeginPlay() override;
public:
	// Is this attach point a trailer or vehicle
	UPROPERTY(BlueprintReadOnly,EditAnywhere,Category=AttachPoint)
	bool IsTrailer;

	// Distance where attach point becomes visible
	UPROPERTY(EditAnywhere,Category=AttachPoint)
	float DrawDistance=500;
	//in KMH prevents deattach in higher speeds
	UPROPERTY(EditAnywhere,Category=AttachPoint)
	float MaxSpeedToDeAttach=2;
	// Distance where attach point can actually attach
	UPROPERTY(EditAnywhere,Category=AttachPoint)
	float AttachDistance=75.f;
	// if you want to handle constraint breaking when de attaching ( for example after playing jack animation ) trun this on. you can handle destruction of constraint yourself by the attach event
	UPROPERTY(EditAnywhere,Category=AttachPoint)
	bool ManualConstraintDestroy=false;
	// Set to zero to disable
	UPROPERTY(EditAnywhere)
	float AngularBreakableLimit=30000000000.0;
	FTimerHandle Handle;

	UPROPERTY(EditAnywhere,Category=AttachPoint)
	float CheckInterval=0.1;
	UFUNCTION()
	void Check();
	UFUNCTION()
	void MoveWheelOwnerShip() const;

	void ResetWheels() const;

	
	UFUNCTION()
	void OnConstraintBreak(int32 Index);
	/**
	 * This toggles the attach calls result OnTrailerAttach
	 * @param FromSave If from save then animations should be ignored and be attached instantly . This has no internal affect just will be passed to events so you can react to it
	 */
	UFUNCTION(BlueprintCallable)
	void OnTrailerButtonPress(bool FromSave);

	
	UPROPERTY(BlueprintReadWrite,Category=AttachPoint)
	UTrailerAttachPoint* OtherAttachPoint;

	UPROPERTY(BlueprintAssignable,Category=AttachPoint)
	FOnTrailerPromptShow OnTrailerPromptShow;


	UPROPERTY(Transient)
	UPhysicsConstraintComponent * ConstraintComponent;


	UPROPERTY(BlueprintAssignable,Category=AttachPoint)
	FOnTrailerAttach OnTrailerAttach;
};
