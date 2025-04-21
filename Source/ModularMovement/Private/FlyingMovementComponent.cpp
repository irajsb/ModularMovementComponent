// Fill out your copyright notice in the Description page of Project Settings.

#include "FlyingMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"


DEFINE_LOG_CATEGORY(LogHeliMvmt)


UFlyingMovementComponent::UFlyingMovementComponent() : Super()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	OnCalculateCustomPhysics.BindUObject(this, &Self::SubstepTick);
	SetIsReplicatedByDefault(true);
}


// Lifecycle & Events ----------------------------------------------------------

void UFlyingMovementComponent::TickComponent(float deltaTime, ELevelTick type, TickFn* fn)
{
	Super::TickComponent(deltaTime, type, fn);

	if (FBodyInstance* body = GetBodyInstance())
		body->AddCustomPhysics(OnCalculateCustomPhysics);
	else {
		UE_LOG(LogTemp,Log,TEXT("Failed to get body instance!"));
	}


	if (bShouldReplicateInput)
	{
		FVector4 CurrentInput(m_Input.Collective,m_Input.Pitch,m_Input.Yaw,m_Input.Roll);
		if (LastInput!=CurrentInput)
		{
			SetInputOnServer(CurrentInput);
			LastInput = CurrentInput;
		}
	}
}

void UFlyingMovementComponent::SetUpdatedComponent(USceneComponent* cmp)
{
	Super::SetUpdatedComponent(cmp);

	PawnOwner = cmp ? Cast<APawn>(cmp->GetOwner()) : nullptr;

	if (auto* mesh = Cast<USkeletalMeshComponent>(cmp))
		mesh->bLocalSpaceKinematics = true;
	else {
		UE_LOG(LogTemp,Log,TEXT("Failed to cast component to USkeletalMeshComponent!"));
		
	}
}

void UFlyingMovementComponent::SubstepTick(float deltaTime, FBodyInstance* body)
{
	UpdateEngineState(deltaTime);
	UpdatePhysicsState(deltaTime, body);
	UpdateSimulation(deltaTime, body);
}

void UFlyingMovementComponent::UpdateEngineState(float deltaTime)
{
	

	FEngineState& State = m_EngineState;

	switch (EnginePhase) {
		case EChopperEngineState::SpoolingUp: {
			State.SpoolAlpha += (1 / SpoolUpTime) * deltaTime;

			auto sinAlpha = CurveSin(State.SpoolAlpha);
			State.PowerAlpha = InverseLerp(sinAlpha, 0.667, 1.0);

			if (State.SpoolAlpha >= 1) {
				State.SpoolAlpha = 1;
				State.PowerAlpha = 1;
				SetEngineState( EChopperEngineState::Running);
				State.RPM = RPM;
			} else {
				State.RPM = RPM * sinAlpha;
			}
		} break;

		case EChopperEngineState::SpoolingDown: {
			State.SpoolAlpha -= (1 / SpoolUpTime) * deltaTime;

			float sinAlpha = CurveSin(State.SpoolAlpha);
			State.PowerAlpha = InverseLerp(sinAlpha, 0.667, 1.0);

			if (State.SpoolAlpha <= 0) {
				State.SpoolAlpha = 0;
				State.PowerAlpha = 0;
				SetEngineState( EChopperEngineState::Off);
				State.RPM = 0;
			} else {
				State.RPM = RPM * sinAlpha;
			}
		} break;

		default: break;
	}
}

void UFlyingMovementComponent::UpdatePhysicsState(float deltaTime, FBodyInstance* body)
{
	FTransform transform = GetOwner()->GetActorTransform();

	FPhysicsCommand::ExecuteRead(body->ActorHandle, [&](FPhysicsActorHandle const& handle)
	{
		float Mass = FPhysicsInterface::GetMass_AssumesLocked(handle);
		FVector COM = FPhysicsInterface::GetComTransform_AssumesLocked(handle).GetLocation();
		FVector LV = body->GetUnrealWorldVelocity_AssumesLocked();
		FVector av = body->GetUnrealWorldAngularVelocityInRadians_AssumesLocked();
		FVector dv = LV - m_PhysicsState.LinearVelocity;
		float aoa = FMath::Asin((Up() | LV) / LV.Size());
		FVector gForce = transform.InverseTransformVector(dv / (k_Gravity * deltaTime));

		m_PhysicsState.Mass = Mass;
		m_PhysicsState.COM = COM;
		m_PhysicsState.LinearVelocity = LV;
		m_PhysicsState.AngularVelocity = av;
		m_PhysicsState.DeltaVelocity = dv;
		m_PhysicsState.GForce = gForce;
		m_PhysicsState.AngleOfAttack = aoa;

		if (LV.Size() < 100)
		{
			m_PhysicsState.CrossSectionalArea = 0;
			return;
		}

		m_PhysicsState.CrossSectionalArea =
			ComputeCrossSectionalArea(body, handle, LV.GetSafeNormal());
	});
}

void UFlyingMovementComponent::UpdateSimulation(float deltaTime, FBodyInstance* body) const
{
	float Mass = m_PhysicsState.Mass;
	FVector COM = m_PhysicsState.COM;
	FVector LV = m_PhysicsState.LinearVelocity;
	FVector av = m_PhysicsState.AngularVelocity;
	float surfArea = m_PhysicsState.CrossSectionalArea;
	float aoa = m_PhysicsState.AngleOfAttack;

	FVector dv = ComputeThrust(COM, Mass);
	FVector drag = ComputeDrag(LV, aoa, surfArea);
	FVector torque = LV.IsNearlyZero(10.f)
		? FVector::ZeroVector
		: ComputeTorque(av, Mass);

	if (DebugPhysics)
		DebugPhysicsSimulation(COM, LV, dv, drag, surfArea);

	if (AeroTorqueInfluence)
		ComputeAeroTorque(LV, Mass, torque);

	body->AddForce(dv + drag);

	
	body->AddTorqueInRadians(torque);
	AddStabilizingTorque(body,0.f);
	
	
}

void UFlyingMovementComponent::AddStabilizingTorque(FBodyInstance* Body,float DeltaTime) const
{
	// Get the physics body
	
    if (m_Input.Pitch!=0.f||m_Input.Roll!=0.f||m_Input.Yaw!=0.f)
    {
	    return;
    }
	// Get current rotation as quaternion
	FQuat CurrentQuat = Body->GetUnrealWorldTransform().GetRotation();
    
	// Extract the yaw rotation (around Z-axis)
	FRotator CurrentRotation = CurrentQuat.Rotator();
	float CurrentYaw = CurrentRotation.Yaw;
    
	// Create target rotation (upright but preserving yaw)
	FRotator TargetRotation(0.0f, CurrentYaw, 0.0f);
	FQuat TargetQuat = TargetRotation.Quaternion();
    
	// Calculate the difference between current and target orientation
	FQuat DeltaQuat = TargetQuat * CurrentQuat.Inverse();
    
	// Convert to axis-angle representation
	FVector Axis;
	float Angle;
	DeltaQuat.ToAxisAndAngle(Axis, Angle);
    
	// Normalize angle
	if (Angle > PI)
	{
		Angle -= 2.0f * PI;
	}
    
	// Calculate torque strength based on deviation from upright
	FVector Torque = Axis *FMath::Clamp(Angle,-0.5,0.5)  * AntiRolloverForce;
	
	// Apply torque to the physics body
	Body->AddTorqueInRadians(Torque); // true = torque is in world space
}


// Physics Calculations --------------------------------------------------------

float UFlyingMovementComponent::ComputeCrossSectionalArea(
	FBodyInstance const* body,
	FPhysicsActorHandle const& handle,
	FVector const& velocityDirection)
	const
{
	FBox bb = FPhysicsInterface::GetBounds_AssumesLocked(handle);
	float extent = bb.GetExtent().GetAbsMax();

	// dp = direction plane: a plane perpendicular to the direction of travel.
	// We'll fire a bunnch of line traces from various points on this plane
	// toward the vehicle, and use the proportion of hits to roughly estimate
	// the cross-sectional area of the vehicle for drag calculations.
	FVector dpCenter = bb.GetCenter() + velocityDirection * extent;
	FVector dpNormal = velocityDirection * -1;
	FVector dpTan, dpBinorm;
	dpNormal.FindBestAxisVectors(dpTan, dpBinorm);

	float step = extent / 16.0;
	FHitResult hit;
	int32 hits = 0, total = 0;

	for (float x = -extent; x < extent; x += step)
	{
		for (float y = -extent; y < extent; y += step)
		{
			++total;

			FVector p1 = dpCenter + (dpTan * x) + (dpBinorm * y);
			FVector p2 = p1 + (dpNormal * extent * 4.0);

			if (body->LineTrace(hit, p1, p2, false))
				++hits;
		}
	}

	return ((float) hits / (float) total) * extent * 4.0;
}


FVector UFlyingMovementComponent::ComputeThrust(FVector const& pos, float Mass) const
{
	

	// Scale the collective input by the current engine power
	float scaledInput = m_Input.Collective * m_EngineState.PowerAlpha;

	// Compute the base thrust magnitude
	float thrust = scaledInput >= 0
		? FMath::Lerp(0.0, -k_Gravity + EnginePower, scaledInput)
		: FMath::Lerp(0.0, k_Gravity - EnginePower, FMath::Abs(scaledInput));

	// Ground effect - increases rotor efficiency when altitude < 80m
	// TODO: Make the ground effect altitude curve configurable
	float agl = GetRadarAltitude();
	float geAlpha = InverseLerp(agl, 80'00, 0);
	float groundEffect = FMath::Clamp(geAlpha * EnginePower * scaledInput, 0, EnginePower);

	// Altitude penalty - decreases rotor efficiency at high altitudes
	float altPenalty = 1.0;
	if (AltitudePenaltyCurve)
		altPenalty = AltitudePenaltyCurve->GetFloatValue(pos.Z / 100.0);
	const float ForceFinalAlpha=LevitatingForceAlpha;//*FMath::Max(Up().Dot(FVector::UpVector), 0.5f);


	FVector ForceVector= Up();
	//rotate force vector towards forward
	ForceVector= FMath::Lerp(ForceVector, Forward(), m_Input.Pitch*0.2);
	ForceVector= FMath::Lerp(ForceVector, Right(), m_Input.Roll*0.1);
	return Mass * ((thrust * altPenalty) + groundEffect) * ForceVector+ Mass*980*ForceVector *m_EngineState.PowerAlpha*ForceFinalAlpha;
}

FVector UFlyingMovementComponent::ComputeDrag(
	FVector const& velocity,
	float aoa,
	float area)
	const 
{
	

	float aoaAbs = FMath::Abs(aoa);
	float cd = 0.0;
	if (DragCoefficientCurve)
	{
		cd = DragCoefficientCurve->GetFloatValue(FMath::RadiansToDegrees(aoaAbs));
	}
	else
	{
		float aoaAlpha = InverseLerp(aoaAbs, 0, PI / 2);
		cd = FMath::Lerp(0.667, 1.5, aoaAlpha);
	}

	float rho = 0.01225; // TODO: Modulate air density by altitude
	float v = velocity.Size() / 15.0;
	float drag = 0.5 * cd * rho * v * v * area;

	// Convert a portion of drag to lift when pitching up (i.e. "cyclic climb")
	float stallAngle = FMath::DegreesToRadians(30);
	float lift = 0.f;
	
	if (aoa < 0 && aoaAbs < stallAngle)
	{
		float ideal = stallAngle * 0.5f;
		float factor = 1.f - (FMath::Abs(aoaAbs - ideal) / ideal);
		lift = factor * drag;
		drag -= lift;
	}

	FVector dragVector = velocity.GetSafeNormal() * -drag;
	FVector liftVector = Up() * lift;

	return dragVector + liftVector;
}

FVector UFlyingMovementComponent::ComputeTorque(FVector const& angularVelocity, float Mass) const
{
	const float PitchAssistance=FMath::Min(1, 1-FMath::Max( (Forward()*m_Input.Pitch).Dot(FVector::DownVector),0)*2);
	const float RollAssistance=FMath::Min(1, 1-FMath::Max( (Right()*m_Input.Roll).Dot(FVector::DownVector),0)*2);
	
	FVector pitch = Right() * m_Input.Pitch * CyclicSensitivity*PitchAssistance;
	FVector roll = Forward() * -m_Input.Roll * CyclicSensitivity*RollAssistance;
	FVector yaw = Up() * m_Input.Yaw * AntiTorqueSensitivity;

	FVector target = pitch + roll + yaw;
	FVector inputTorque = target - angularVelocity;

	return inputTorque * Mass * 1'000'00 * Agility;
}

void UFlyingMovementComponent::ComputeAeroTorque(
	FVector const& velocity,
	float Mass,
	FVector& inout_torque)
	const
{
	APawn* pawn = GetPawn();
	if (!ensure(pawn)) return;

	FVector vRel = pawn
		->GetTransform()
		.InverseTransformVector(velocity)
		.RotateAngleAxis(15, FVector::RightVector);

	float thetaZ = FMath::Atan2(vRel.Y, vRel.X);
	float thetaX = FMath::Atan2(vRel.Z, vRel.Y);

	FVector latVel { vRel.X, vRel.Y, 0 };
	float influence = AeroTorqueInfluence->GetFloatValue(latVel.Size() / 100.0);

	inout_torque += (Up() * thetaZ * Mass * 120 * 1000 * influence);
	inout_torque += (Right() * -thetaX * Mass * 120 * 50 * influence);
}


// Utility ---------------------------------------------------------------------

APawn* UFlyingMovementComponent::GetPawn() const 
{
	if (!UpdatedComponent) return nullptr;
	return Cast<APawn>(UpdatedComponent->GetOwner());
}

FBodyInstance* UFlyingMovementComponent::GetBodyInstance() const 
{
	if (!UpdatedComponent) return nullptr;

	auto* cmp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (!cmp) return nullptr;

	return cmp->GetBodyInstance();
}

FVector UFlyingMovementComponent::Forward() const 
{
	APawn* pawn = GetPawn();
	if (ensure(pawn != nullptr))
		return pawn->GetActorForwardVector();

	return FVector::ForwardVector;
}

FVector UFlyingMovementComponent::Right() const 
{
	APawn* pawn = GetPawn();
	if (ensure(pawn != nullptr))
		return pawn->GetActorRightVector();

	return FVector::RightVector;
}

FVector UFlyingMovementComponent::Up() const 
{
	APawn* pawn = GetPawn();
	if (ensure(pawn != nullptr))
		return pawn->GetActorUpVector();

	return FVector::UpVector;
}


// Debug -----------------------------------------------------------------------

void UFlyingMovementComponent::DebugPhysicsSimulation(
	FVector const& centerOfMass,
	FVector const& linearVelocity,
	FVector const& thrust,
	FVector const& drag,
	float crossSectionalArea)
	const
{
	// Draw a sphere around the center of Mass
	DrawDebugSphere(GetWorld(), centerOfMass, 35, 8, FColor::White, false, -1, 128, 5);

	// Draw a line indicating linear velocity
	DrawDebugLine(
		GetWorld(),
		centerOfMass,
		centerOfMass + linearVelocity,
		FColor::Cyan,
		false, -1, 0,
		(linearVelocity.Size() / 200.0));

	// Draw a line for the thrust vector
	DrawDebugLine(
		GetWorld(),
		centerOfMass,
		centerOfMass + (thrust / 1000.0),
		FColor::Green,
		false, -1, 0,
		(thrust.Size() / 50'000.0)
	);

	// Draw a circle to represent the cross-sectional area
	FVector circleY, circleZ;
	linearVelocity.GetSafeNormal().FindBestAxisVectors(circleY, circleZ);
	DrawDebugCircle(
		GetWorld(),
		centerOfMass,
		FMath::Sqrt((crossSectionalArea * 100.0) / PI),
		32,
		FColor::Magenta,
		false,
		-1,
		255,
		5,
		circleY,
		circleZ
	);

	// Draw a line for the drag vector
	DrawDebugLine(
		GetWorld(),
		centerOfMass,
		centerOfMass + (drag / 500.0),
		FColor::Red,
		false, -1, 0,
		(drag.Size() / 50'000.0)
	);
}

void UFlyingMovementComponent::SetEngineStateOnServer_Implementation(EChopperEngineState State)
{
	SetEngineState(State);
}

void UFlyingMovementComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFlyingMovementComponent,EnginePhase)
}

void UFlyingMovementComponent::SetInputOnServer_Implementation(FVector4 Input)
{
	m_Input.Collective=Input.X;
	m_Input.Pitch=Input.Y;
	m_Input.Yaw=Input.Z;
	m_Input.Roll=Input.W;
	
}


// Blueprint Getters -----------------------------------------------------------

float UFlyingMovementComponent::GetCurrentRPM() const
{
	return m_EngineState.RPM;
}

float UFlyingMovementComponent::GetCurrentCollective() const
{
	return m_Input.Collective;
}

FVector2D UFlyingMovementComponent::GetCurrentCyclic() const 
{
	return { m_Input.Roll, m_Input.Pitch };
}

float UFlyingMovementComponent::GetCurrentTorque() const
{
	return m_Input.Yaw;
}

FVector UFlyingMovementComponent::GetVelocity() const 
{
	return m_PhysicsState.LinearVelocity;
}

float UFlyingMovementComponent::GetLateralAirspeed() const
{
	FVector LV = m_PhysicsState.LinearVelocity;
	FVector latVel { LV.X, LV.Y, 0 };

	return latVel.Size();
}

float UFlyingMovementComponent::GetLateralAirspeedKnots() const
{
	return GetLateralAirspeed() * k_CmPerSecToKnots;
}

float UFlyingMovementComponent::GetVerticalAirspeed() const
{
	return m_PhysicsState.LinearVelocity.Z;
}

float UFlyingMovementComponent::GetHeadingDegrees() const
{
	FVector direction = FVector::VectorPlaneProject(Forward(), FVector::UpVector);
	direction.Normalize();

	return FMath::RadiansToDegrees(FMath::Atan2(-direction.Y, -direction.X)) + 180.0;
}

float UFlyingMovementComponent::GetRadarAltitude() const
{
	APawn* pawn = GetPawn();
	if (pawn == nullptr) return UE_BIG_NUMBER;

	auto params = FCollisionQueryParams::DefaultQueryParam;
	params.AddIgnoredActor(pawn);

	FVector start = m_PhysicsState.COM;
	FVector end = start + FVector::DownVector * 2000'00.f;
	FHitResult hit;

	if (GetWorld()->LineTraceSingleByChannel(hit, start, end, ECC_WorldStatic, params))
		return FVector::Dist(start, hit.Location);

	return UE_BIG_NUMBER;
}


// Blueprint Methods -----------------------------------------------------------

void UFlyingMovementComponent::HoldStarter(float StartTime)
{
	if (EnginePhase != EChopperEngineState::Running)
		SetEngineState(  EChopperEngineState::SpoolingUp);
	
}

void UFlyingMovementComponent::StopEngine()
{
	if (EnginePhase != EChopperEngineState::Off)
		SetEngineState(  EChopperEngineState::SpoolingDown);
}

void UFlyingMovementComponent::SetCollectiveInput(float value)
{
	if (value > 0 && EnginePhase == EChopperEngineState::Off)
		HoldStarter(0.f);

	m_Input.Collective = value;
}

void UFlyingMovementComponent::SetPitchInput(float value)
{
	m_Input.Pitch = value;
}

void UFlyingMovementComponent::SetRollInput(float value)
{
	m_Input.Roll = value;
}

void UFlyingMovementComponent::SetYawInput(float value)
{
	m_Input.Yaw = value;
}



void UFlyingMovementComponent::SetEngineState(EChopperEngineState State)
{
	

	EnginePhase=State;
	if (GetPawn())
	{
		if (GetPawn()->GetOwner())
		{
			if (GetOwner()->GetLocalRole()<ROLE_Authority)
			{
				SetEngineStateOnServer(EnginePhase);
			}
		}
	}
}


#undef HELI_LOG
#undef HELI_WARN