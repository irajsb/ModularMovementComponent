//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "TankTrackComponent.h"
#include "Components/StaticMeshComponent.h"
#include "IdlerComponent.h"
#include "TankTireModel.h"
#include "Engine/StaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

UTankTrackComponent::UTankTrackComponent()
{
	SetClosedLoop(true);
	SetTickGroup(TG_PostPhysics);
	//Enable tick
	PrimaryComponentTick.bCanEverTick = true;
	#if WITH_EDITOR
	ScaleVisualizationWidth=30.f;
	bShouldVisualizeScale=true;
	#endif
	
}

void UTankTrackComponent::PostInitProperties()
{
	Super::PostInitProperties();
	RebuildSplines(0.f, false);
}

void UTankTrackComponent::SetupComponents(TArray<UTrackableComponent*> InWheels)
{
	Wheels = InWheels;
	RebuildSplines(0.f, true);
	OriginalSplineLen = GetSplineLength();
	InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(
			GetOwner()->AddComponentByClass(UInstancedStaticMeshComponent::StaticClass(), false, FTransform(), true));

	if (InstancedStaticMeshComponent)
	{
	
		
		
	
		InstancedStaticMeshComponent->SetStaticMesh(TrackMesh);
		InstancedStaticMeshComponent->SetMaterial(0, TrackMesh->GetMaterial(0));
	
		UpdateInstanceTransforms.AddUninitialized(NumOfMeshesInTrack);
	

		for (int32 Index = 0; Index < NumOfMeshesInTrack; Index++)
		{
			FTransform Transform = GetTransformAtTime(Index / static_cast<float>(NumOfMeshesInTrack),
													  ESplineCoordinateSpace::Local, true);
			Transform.SetScale3D(Scale);

		
			UpdateInstanceTransforms[Index] = Transform;
		
			InstancedStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}


	GetOwner()->FinishAddComponent(InstancedStaticMeshComponent,false,FTransform());

	UpdateMeshes(0.f, true);
}

void UTankTrackComponent::RebuildSplines(float DeltaTime, bool Editor)
{



	 LenDiff=OriginalSplineLen-GetSplineLength();

	UTrackableComponent* Sprocket=nullptr;
	int SprocketIndex=0;
	
	int NumSwayComponents=0;
	
	for(auto Trackable : Wheels)
	{
		for(auto Contact:Trackable->ContactPoints)
		{
			if(Contact.MaxSway>0.f)
			{
				NumSwayComponents++;
			}
		}
	}

	//Calculate Sway per component

	
	TrackSpeed = 0.f;

	if (Wheels.Num() == 0)
	{
		return;
	}

	ClearSplinePoints(false);

	for (int32 Index = 0; Index != Wheels.Num(); ++Index)
	{

		float Sway=0.f;
		
		UTrackableComponent* Trackable = Wheels[Index];

		if(!Editor)
		{
			Sway=LenDiff/NumSwayComponents;
		}

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
					if (const auto Obj = Wheel->WheelState.WheelSetupClass.Get()->ClassDefaultObject)
					{
						Radius = Cast<UModularVehicleWheelData>(Obj)->WheelRadius;
					}
				}
				
				if (TrackSpeed == 0)
				{
					if(Wheel->WheelState.WheelSetup)
					{
						if(auto TireModel=Cast<UTankTireModel>(Wheel->WheelState.WheelSetup->TireModel))
						{
							SprocketRadius=TireModel->SprocketRadius;
							TrackSpeed = Wheel->WheelState.AngularVelocity * SprocketRadius ;
						}
					}
					SprocketRatio=Radius/SprocketRadius;
				}
			}
		}
		else
		{
			const auto Idler = Cast<UIdlerComponent>(Trackable);
			// If it's an Idler, get the radius directly
			Radius = Idler->Radius;

			
			
		}

		for (int ContactIndex=0;ContactIndex!=   Trackable->ContactPoints.Num();++ContactIndex)
		{
			// Calculate the position and tangent on the wheel or idler's surface
			const auto ContactPoint=Trackable->ContactPoints[ContactIndex];
			bool HasCustomTangent=false;
			

			
				if(ContactPoint.CustomTangent!=FVector::ZeroVector)
				{
					HasCustomTangent=true;
					
				}

			
			if(Trackable->IsSprocket&&ContactIndex==1)
			{
				Sprocket=Trackable;
				SprocketIndex=GetNumberOfSplinePoints();
			}
			Sway=FMath::Clamp(Sway,-ContactPoint.MaxSway,ContactPoint.MaxSway);
			const FVector2D RelativePosition = ContactPoint.ContactPoint * Radius+FVector2D(0,-Sway);
			const FVector Position = Trackable->GetRelativeLocation();
			const FVector ContactPointVector = FVector(RelativePosition.X, 0, RelativePosition.Y + ZOffset) / GetOwner()
				->GetActorScale3D() - (IsWheel ? WheelOffset : IdlerOffset);


			// Add the spline point with the calculated position and tangent
			AddSplinePoint(Position + ContactPointVector, ESplineCoordinateSpace::Local, false);

			
			//Find angle between contact point and up

			if(HasCustomTangent)
			{
					
				SetRotationAtSplinePoint(GetNumberOfSplinePoints() - 1, FRotator(ContactPoint.Angle, 0, 0),
									 ESplineCoordinateSpace::Local, false);
				SetTangentAtSplinePoint(GetNumberOfSplinePoints() - 1, ContactPoint.CustomTangent,
													ESplineCoordinateSpace::Local, false);
				
			}
			else
			{
			
			const float Angle = FMath::Sign(ContactPoint.ContactPoint.X + 0.00001) * FMath::RadiansToDegrees(
				FMath::Acos(FVector2D::DotProduct(ContactPoint.ContactPoint, FVector2D(0, -1))));

			SetRotationAtSplinePoint(GetNumberOfSplinePoints() - 1, FRotator(Angle, 0, 0),
			                         ESplineCoordinateSpace::Local, false);
			SetTangentAtSplinePoint(GetNumberOfSplinePoints() - 1, FRotator(Angle, 0, 0).Vector() * Radius,
			                        ESplineCoordinateSpace::Local, false);
			
		}
			}
	}

	
	UpdateSpline();
	LenDiff=OriginalSplineLen-GetSplineLength();

	if(Sprocket)
	{
		
		if(Editor)
		{
				
			OriginalTime=GetTimeAtDistanceAlongSpline(GetDistanceAlongSplineAtSplinePoint(SprocketIndex));
			OriginalSprocketContactPoint=GetLocationAtTime(OriginalTime,ESplineCoordinateSpace::Local,true);
		}else
		{
			const FVector IdlerLocation=Sprocket->GetRelativeLocation();
			const FVector CurrentContactPoint=GetLocationAtTime(OriginalTime,ESplineCoordinateSpace::Local,true);
			//Calculate Angle diff
			FVector OriginalVector = OriginalSprocketContactPoint - IdlerLocation;
			FVector CurrentVector = CurrentContactPoint - IdlerLocation;
			// Normalize the vectors to get unit vectors
			OriginalVector.Normalize();
			CurrentVector.Normalize();
			// Calculate the dot product
			float DotProduct = FVector::DotProduct(OriginalVector, CurrentVector);

			// Clamp the dot product to handle numerical precision issues
			DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);

			// Calculate the angle in radians using the dot product formula
			SplinePointDiff = FMath::Acos(DotProduct);
		}
	}
		
}

void UTankTrackComponent::UpdateMeshes(float DeltaTime, bool Editor)
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
		float SplineTime = (Index / static_cast<float>(NumOfMeshesInTrack) - CurrentOffset+ConstantOffset);
		
		if (SplineTime < 0.f)
		{
			SplineTime += 1.f;
		}


		FTransform Transform=GetTransformAtTime(SplineTime ,  ESplineCoordinateSpace::Local, true);
		

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

	
	


	

#if ENGINE_MINOR_VERSION >3
	if(InstancedStaticMeshComponent->GetInstanceCount()!=NumOfMeshesInTrack)
	{
#else
	if(InstancedStaticMeshComponent->PerInstanceIds.Num()!=NumOfMeshesInTrack)
	{
#endif
		InstancedStaticMeshComponent->ClearInstances();
		for (int32 Index = 0; Index < NumOfMeshesInTrack; Index++)
		{
			InstancedStaticMeshComponent->AddInstance(UpdateInstanceTransforms[Index]);
		
		}
	}else
	{
		for (int32 Index = 0; Index < NumOfMeshesInTrack; Index++)
		{
			InstancedStaticMeshComponent->UpdateInstanceTransform(Index,UpdateInstanceTransforms[Index]);
		
		}
	}
	InstancedStaticMeshComponent->MarkRenderStateDirty();
}

void UTankTrackComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//Log Delta Time

	RebuildSplines(DeltaTime, false);
	UpdateMeshes(DeltaTime, false);
}

void UTankTrackComponent::BeginPlay()
{

	if(NumOfMeshesInTrack!=UpdateInstanceTransforms.Num())
	{
		UpdateInstanceTransforms.AddUninitialized(NumOfMeshesInTrack);
	}
	InstancedStaticMeshComponent->ClearInstances();
	
	Super::BeginPlay();

}


#if WITH_EDITOR

void UTankTrackComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateInstanceTransforms.Reset();
	UpdateInstanceTransforms.AddUninitialized(NumOfMeshesInTrack);
	if(InstancedStaticMeshComponent)
	{
		InstancedStaticMeshComponent->ClearInstances();
		RebuildSplines(0.0f, true);
		UpdateMeshes(0.1f, true);
	}
}


#endif