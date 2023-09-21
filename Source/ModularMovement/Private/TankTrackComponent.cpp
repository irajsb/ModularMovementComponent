//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "TankTrackComponent.h"
#include "Components/StaticMeshComponent.h"
#include "IdlerComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"

UTankTrackComponent::UTankTrackComponent()
{
	SetClosedLoop(true);
	SetTickGroup(TG_PostPhysics);
	//Enable tick
	PrimaryComponentTick.bCanEverTick = true;
}

void UTankTrackComponent::PostInitProperties()
{
	Super::PostInitProperties();
	RebuildSplines(0.f);
}

void UTankTrackComponent::SetupComponents(TArray<UTrackableComponent*> InWheels)
{
	Wheels = InWheels;
	RebuildSplines(0.f);
}

void UTankTrackComponent::RebuildSplines(float DeltaTime)
{
	TrackSpeed = 0.f;

	if (Wheels.Num() == 0)
	{
		return;
	}

	ClearSplinePoints(false);

	for (int32 Index = 0; Index != Wheels.Num(); ++Index)
	{
		UTrackableComponent* Trackable = Wheels[Index];

		if (!Trackable)
		{
			return;
		}

		float Radius = 0.f;
		float ZOffset = 0.f;

		const bool IsWheel = Trackable->IsA(UModularWheel::StaticClass());
		// Check if it's an Idler or a Wheel
		if (IsWheel)
		{
			if (const auto Wheel = Cast<UModularWheel>(Trackable))
			{
				// If it's a Wheel, get the radius from WheelSetup or WheelSetupClass
				if (Cast<UModularWheel>(Trackable)->WheelState.WheelSetup)
				{
					Radius = Wheel->WheelState.WheelSetup->WheelRadius;
					ZOffset = Wheel->WheelState.PreviousLocation.Z;
				}
				else
				{
					if (const auto Obj = Wheel->WheelState.WheelSetupClass.LoadSynchronous()->ClassDefaultObject)
					{
						Radius = Cast<UModularVehicleWheelData>(Obj)->WheelRadius;
					}
				}
				if (TrackSpeed == 0)
				{
					TrackSpeed = Wheel->WheelState.AngularVelocity * Radius;
				}
			}
		}
		else
		{
			const auto Idler = Cast<UIdlerComponent>(Trackable);
			// If it's an Idler, get the radius directly
			Radius = Idler->Radius;
		}

		for (auto ContactPoint : Trackable->ContactPoints)
		{
			// Calculate the position and tangent on the wheel or idler's surface
			const FVector2D RelativePosition = ContactPoint * Radius;
			const FVector Position = Trackable->GetRelativeLocation();
			const FVector ContactPointVector = FVector(RelativePosition.X, 0, RelativePosition.Y + ZOffset) / GetOwner()
				->GetActorScale3D() - (IsWheel ? WheelOffset : IdlerOffset);


			// Add the spline point with the calculated position and tangent
			AddSplinePoint(Position + ContactPointVector, ESplineCoordinateSpace::Local, false);


			//Find angle between contact point and up

			const float Angle = FMath::Sign(ContactPoint.X + 0.00001) * FMath::RadiansToDegrees(
				FMath::Acos(FVector2D::DotProduct(ContactPoint, FVector2D(0, -1))));

			SetRotationAtSplinePoint(GetNumberOfSplinePoints() - 1, FRotator(Angle, 0, 0),
			                         ESplineCoordinateSpace::Local, false);
			SetTangentAtSplinePoint(GetNumberOfSplinePoints() - 1, FRotator(Angle, 0, 0).Vector() * Radius,
			                        ESplineCoordinateSpace::Local, false);
		}
	}

	UpdateSpline();
}

void UTankTrackComponent::UpdateMeshes(float DeltaTime)
{
	CurrentOffset += TrackSpeed * DeltaTime / GetSplineLength();


	if (CurrentOffset > 1.f)
	{
		CurrentOffset -= 1.f;
	}
	if (CurrentOffset < 0)
	{
		CurrentOffset += 1.f;
	}

	if (!InstancedStaticMeshComponent)
	{
		return;
	}

	for (int32 Index = 0; Index < NumOfMeshesInTrack; Index++)
	{
		float SplineTime = (Index / static_cast<float>(NumOfMeshesInTrack) - CurrentOffset);
		if (SplineTime < 0.f)
		{
			SplineTime += 1.f;
		}

		FTransform Transform = GetTransformAtTime(SplineTime, ESplineCoordinateSpace::Local, true);

		//Roll the rotation 180 degrees


		FVector UpVector = GetUpVectorAtTime(SplineTime, ESplineCoordinateSpace::Local, true);


		//convert to forward vector by swapping z and X
		UpVector = FVector(-UpVector.Z, 0.f, UpVector.X);


		FRotator Rot = UpVector.ToOrientationQuat().Rotator();


		if (Rot.Yaw > 90)
		{
			Rot.Pitch = 180 - Rot.Pitch;
		}
		Rot.Yaw = 0.f;
		Transform.SetRotation(Rot.Quaternion());


		Transform.SetScale3D(Scale);

		UpdateInstanceTransforms[Index] = Transform;
	}
	InstancedStaticMeshComponent->UpdateInstances(UpdateInstanceIds, UpdateInstanceTransforms,
	                                              UpdateInstancePreviousTransforms, 0, {});


	for (int32 Index = 0; Index < NumOfMeshesInTrack; Index++)
	{
		UpdateInstancePreviousTransforms[Index] = UpdateInstanceTransforms[Index];
	}
}

void UTankTrackComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//Log Delta Time

	RebuildSplines(DeltaTime);
	UpdateMeshes(DeltaTime);
}

void UTankTrackComponent::BeginPlay()
{
	Super::BeginPlay();
	if (TrackMesh)
	{
		InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(
			GetOwner()->AddComponentByClass(UInstancedStaticMeshComponent::StaticClass(), false, FTransform(), false));
		InstancedStaticMeshComponent->SetStaticMesh(TrackMesh);
		InstancedStaticMeshComponent->SetMaterial(0, TrackMesh->GetMaterial(0));
		UpdateInstanceIds.AddUninitialized(NumOfMeshesInTrack);
		UpdateInstanceTransforms.AddUninitialized(NumOfMeshesInTrack);
		UpdateInstancePreviousTransforms.AddUninitialized(NumOfMeshesInTrack);
		InNumCustomDataFloats = NumOfMeshesInTrack;
		CustomFloatData.AddUninitialized(NumOfMeshesInTrack);

		for (int32 Index = 0; Index < NumOfMeshesInTrack; Index++)
		{
			FTransform Transform = GetTransformAtTime(Index / static_cast<float>(NumOfMeshesInTrack),
			                                          ESplineCoordinateSpace::Local, true);
			Transform.SetScale3D(Scale);

			UpdateInstanceIds[Index] = Index;
			UpdateInstanceTransforms[Index] = Transform;
			UpdateInstancePreviousTransforms[Index] = Transform;
			InstancedStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	UpdateMeshes(0.f);
}
