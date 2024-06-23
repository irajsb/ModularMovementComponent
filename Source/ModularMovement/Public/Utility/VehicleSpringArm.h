//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"

#include "Engine/EngineTypes.h"
#include "Camera/CameraShakeBase.h"
#include "Components/SceneComponent.h"
#include "VehicleSpringArm.generated.h"

/**
This spring arm only lags on Z axis . Has ability to play shakes on hitting the ground . Automatically rotates to direction that vehicle is moving with support to manually
*Rotate the camera . Use setcooldown to allow user to manually move the cam . once cooldown is finished camera will auto orient again .
 */

UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent), hideCategories = (Mobility))
class MODULARMOVEMENT_API UVehicleSpringArm : public USceneComponent
{
	GENERATED_UCLASS_BODY()
	/** Natural length of the spring arm when there are no collisions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float TargetArmLength;

	/** offset at end of spring arm; use this instead of the relative offset of the attached component to ensure the line trace works as desired */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	FVector SocketOffset;

	/** Offset at start of spring, applied in world space. Use this if you want a world-space offset from the parent component instead of the usual relative-space offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	FVector TargetOffset;

	

	/** How big should the query probe sphere be (in unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraCollision, meta = (editcondition = "bDoCollisionTest"))
	float ProbeSize;

	/** Collision channel of the query probe (defaults to ECC_Camera) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraCollision, meta = (editcondition = "bDoCollisionTest"))
	TEnumAsByte<ECollisionChannel> ProbeChannel;

	/** If true, do a collision test using ProbeChannel and ProbeSize to prevent camera clipping into level.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraCollision)
	uint32 bDoCollisionTest : 1;

	/**
	 * If this component is placed on a pawn, should it use the view/control rotation of the pawn where possible?
	 * When disabled, the component will revert to using the stored RelativeRotation of the component.
	 * Note that this component itself does not rotate, but instead maintains its relative rotation to its parent as normal,
	 * and just repositions and rotates its children as desired by the inherited rotation settings. Use GetTargetRotation()
	 * if you want the rotation target based on all the settings (UsePawnControlRotation, InheritPitch, etc).
	 *
	 * @see GetTargetRotation(), APawn::GetViewRotation()
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSettings)
	uint32 bUsePawnControlRotation : 1;

	/** Should we inherit pitch from parent component. Does nothing if using Absolute Rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSettings)
	uint32 bInheritPitch : 1;

	/** Should we inherit yaw from parent component. Does nothing if using Absolute Rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSettings)
	uint32 bInheritYaw : 1;

	/** Should we inherit roll from parent component. Does nothing if using Absolute Rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSettings)
	uint32 bInheritRoll : 1;

	/**
	 * If true, camera lags behind target position to smooth its movement.
	 * @see CameraLagSpeed
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag)
	uint32 bEnableCameraLag : 1;

	/**
	 * If true, camera lags behind target rotation to smooth its movement.
	 * @see CameraRotationLagSpeed
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag)
	uint32 bEnableCameraRotationLag : 1;

	/**
	 * If bUseCameraLag Substepping is true, sub-step camera damping so that it handles fluctuating frame rates well (though this comes at a cost).
	 * @see CameraLagMaxTimeStep
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag, AdvancedDisplay)
	uint32 bUseCameraLagSubstepping : 1;

	/**
	 * If true and camera location lag is enabled, draws markers at the camera target (in green) and the lagged position (in yellow).
	 * A line is drawn between the two locations, in green normally but in red if the distance to the lag target has been clamped (by CameraLagMaxDistance).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag)
	uint32 bDrawDebugLagMarkers : 1;

	/** If bEnableCameraLag is true, controls how quickly camera reaches target position. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag,
		meta = (editcondition = "bEnableCameraLag", ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax =
			"1000.0"))
	float CameraLagZSpeed = 10;


	/** If bEnableCameraRotationLag is true, controls how quickly camera reaches target position. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag,
		meta = (editcondition = "bEnableCameraRotationLag", ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax
			= "1000.0"))
	float CameraRotationLagSpeed;

	/** Max time step used when sub-stepping camera lag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag, AdvancedDisplay,
		meta = (editcondition = "bUseCameraLagSubstepping", ClampMin = "0.005", ClampMax = "0.5", UIMin = "0.005", UIMax
			= "0.5"))
	float CameraLagMaxTimeStep;

	/** Max distance the camera target may lag behind the current location. If set to zero, no max distance is enforced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag,
		meta = (editcondition = "bEnableCameraLag", ClampMin = "0.0", UIMin = "0.0"))
	float CameraLagMaxDistance;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float MinPitch=10.f;
	/**
	 * Get the target rotation we inherit, used as the base target for the boom rotation.
	 * This is derived from attachment to our parent and considering the UsePawnControlRotation and absolute rotation flags.
	 */
	UFUNCTION(BlueprintCallable, Category = SpringArm)
	FRotator GetTargetRotation() const;

	/** Get the position where the camera should be without applying the Collision Test displacement */
	UFUNCTION(BlueprintCallable, Category = CameraCollision)
	FVector GetUnfixedCameraPosition() const;

	/** Is the Collision Test displacement being applied? */
	UFUNCTION(BlueprintCallable, Category = CameraCollision)
	bool IsCollisionFixApplied() const;

	/** Temporary variables when applying Collision Test displacement to notify if its being applied and by how much */
	bool bIsCameraFixed = false;
	FVector UnfixedCameraPosition;

	/** Temporary variables when using camera lag, to record previous camera position */
	FVector PreviousDesiredLoc;
	FVector PreviousArmOrigin;
	/** Temporary variable for lagging camera rotation, for previous rotation */
	FRotator PreviousDesiredRot;

	// UActorComponent interface
	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PostLoad() override;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	// End of UActorComponent interface

	// USceneComponent interface
	virtual bool HasAnySockets() const override;
	virtual FTransform
	GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const override;
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const override;
	// End of USceneComponent interface

	/** The name of the socket at the end of the spring arm (looking back towards the spring arm origin) */
	static const FName SocketName;

	/** Returns the desired rotation for the spring arm, before the rotation constraints such as bInheritPitch etc are enforced. */
	virtual FRotator GetDesiredRotation() const;

protected:
	/** Cached component-space socket location */
	FVector RelativeSocketLocation;
	/** Cached component-space socket rotation */
	FQuat RelativeSocketRotation;

protected:
	/** Updates the desired arm location, calling BlendLocations to do the actual blending if a trace is done */
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime);

	/**
	 * This function allows subclasses to blend the trace hit location with the desired arm location;
	 * by default it returns bHitSomething ? TraceHitLocation : DesiredArmLocation
	 */
	virtual FVector BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation,
	                               bool bHitSomething, float DeltaTime);

	//Vehicle spring arm additions
public :
	//When input recevied do pause auto camera movement and reduce lag
	UFUNCTION(BlueprintCallable,Category=Behaviour)
	void SetCooldown(float In);


	// Camera will face target speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleSpringArm)
	bool AutoCorrect=true;
	// If user had any input pause auto correct for few seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleSpringArm)
	bool AutoDetectInput=true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleSpringArm)
	float PauseSeconds=3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleSpringArm)
	float PauseSensitivity=0.998;
	//Speed when auto correct starts ( auto correct rotates camera to vehicle movement direction)
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	float AutoCorrectMinSpeedRange = 300;
	//Speed that Auto Correct reaches its maximum effect
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	float AutoCorrectMaxSpeedRange = 3000;
	//Ignore pitch when auto orienting camera 
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	bool IgnorePitch = true;
	//For smoothing result
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	float AutoCorrectInterpolationStrength = 30;
	//Maximum arm change when accelerating or braking
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	float MaxArmLenChange = 200;
	//Min acceleration ( negative means  braking and shortens the arm len )
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	float MinAcceleration = -1600;
	//Max acceleration when arm reaches max len 
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	float MaxAcceleration = 1600;
	// Smooth out arm movement 
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	float ArmLenInterpolationSpeed = 2;
	//Min time airborne to play camera shake 
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	float MinAirborneTimeForCameraShake = 0.5;
	//Camera shake to play 
	UPROPERTY(EditAnyWhere,Category=VehicleSpringArm)
	TSubclassOf<UCameraShakeBase> CameraShake;

private:
	//Cooldown to let user handle spring arm manually
	float CurrentCooldown;
	//Speed to calculate acceleration
	float PreviousSpeed;
	//Curent arm len for interpolation
	float CurrentArmLen;
	//To keep track of airborne time
	float AirborneTime = 0.f;


	FRotator LastAutoRot;
};
