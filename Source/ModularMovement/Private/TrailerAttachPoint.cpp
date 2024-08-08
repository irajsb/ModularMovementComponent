// Fill out your copyright notice in the Description page of Project Settings.


#include "TrailerAttachPoint.h"

#include "ModularMovement.h"
#include "ModularMovementComponent.h"
#include "ModularVehicleFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

void UTrailerAttachPoint::BeginPlay()
{
	Super::BeginPlay();


	SetHiddenInGame(true);

	if (!IsTrailer)
	{
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &UTrailerAttachPoint::Check, 0.1, true);
	}
}

void UTrailerAttachPoint::Check()
{
	

	if (ConstraintComponent)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			if (UModularMovementComponent* MC = Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
			{
				OnTrailerPromptShow.Broadcast(
					FMath::Abs(UModularVehicleFunctionLibrary::GetForwardSpeedKMH(MC)) < MaxSpeedToDeAttach, true);
			}
		}
	}
	else
	{	OtherAttachPoint = nullptr;
		SetHiddenInGame(true);
		 CanAttach = false;
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), Actors);
		for (auto Actor : Actors)
		{
			if(Actor!=GetOwner())
			{
				if (auto AttachPoint = static_cast<UTrailerAttachPoint*>(Actor->GetComponentByClass(StaticClass())))
				{
					const float Dist = FVector::Distance(AttachPoint->GetComponentLocation(), GetComponentLocation());
					if (Dist < DrawDistance && !OtherAttachPoint)
					{
						OtherAttachPoint = AttachPoint;
						if (Dist < AttachDistance)
						{
							CanAttach = true;
						}
					}
					else
					{
						AttachPoint->SetHiddenInGame(true);
					}
				}
			}
		}

		if (OtherAttachPoint)
		{
			SetHiddenInGame(false);
			OtherAttachPoint->SetHiddenInGame(false);
			OnTrailerPromptShow.Broadcast(CanAttach, false);
		}
	}
}

void UTrailerAttachPoint::MoveWheelOwnerShip() const
{
	if (OtherAttachPoint)
	{
		if (const APawn* Pawn = Cast<APawn>(OtherAttachPoint->GetOwner()))
		{
			if (auto MC = Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
			{
				MC->SetActive(false);

				/// move wheels from trailer to my movement component 
				if (const APawn* MyPawn = Cast<APawn>(GetOwner()))
				{
					if (auto MyMC = Cast<UModularMovementComponent>(MyPawn->GetMovementComponent()))
					{
						MyMC->UpdateComponents(MC->GetWheels());
						MyMC->ActorsToIgnore.AddUnique(OtherAttachPoint->GetOwner());
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogModularVehicle, Error, TEXT("OtherAttachPoint not valid in MoveWheelOwnerShip"))
	}
}

void UTrailerAttachPoint::ResetWheels() const
{
	if (OtherAttachPoint)
	{
		if (const APawn* Pawn = Cast<APawn>(OtherAttachPoint->GetOwner()))
		{
			if (auto MC = Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
			{
				MC->SetActive(true);

				/// move wheels from trailer to my movement component 
				if (const APawn* MyPawn = Cast<APawn>(GetOwner()))
				{
					if (auto MyMC = Cast<UModularMovementComponent>(MyPawn->GetMovementComponent()))
					{
						MyMC->UpdateComponents({});
						MyMC->ActorsToIgnore.Remove(OtherAttachPoint->GetOwner());
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogModularVehicle, Error, TEXT("OtherAttachPoint not valid in ResetWheels"))
	}
}

void UTrailerAttachPoint::OnConstraintBreak(int32 Index)
{
	OnTrailerButtonPress(false); 
}

void UTrailerAttachPoint::OnTrailerButtonPress(bool FromSave)
{
	if (ConstraintComponent)
	{
		OnTrailerAttach.Broadcast(false, false, ConstraintComponent);
		if(OtherAttachPoint)
		{
			OtherAttachPoint->OnTrailerAttach.Broadcast(false, false, ConstraintComponent);
		}

		if (!ManualConstraintDestroy)
		{
			ConstraintComponent->BreakConstraint();
		}
		ConstraintComponent = nullptr;

		ResetWheels();
	}
	else
	{
		if (OtherAttachPoint&&CanAttach)
		{
			SetHiddenInGame(true);
			OtherAttachPoint->SetHiddenInGame(true);
			MoveWheelOwnerShip();

			if (const auto Comp = Cast<UPhysicsConstraintComponent>(
				GetOwner()->AddComponentByClass(UPhysicsConstraintComponent::StaticClass(), false,
				                                GetRelativeTransform(), false)))
			{
				ConstraintComponent = Comp;
				ConstraintComponent->OnConstraintBroken.AddDynamic(this, &UTrailerAttachPoint::OnConstraintBreak);
			}
			if(AngularBreakableLimit>0)
			{
				ConstraintComponent->SetAngularBreakable(true,AngularBreakableLimit);
			}

			ConstraintComponent->SetConstrainedComponents(Cast<UPrimitiveComponent>(OtherAttachPoint->GetAttachParent()),NAME_None,Cast<UPrimitiveComponent>(GetAttachParent()),NAME_None);
			ConstraintComponent->SetConstraintReferencePosition(EConstraintFrame::Frame1,OtherAttachPoint->GetRelativeLocation());
			ConstraintComponent->SetAngularSwing1Limit(ACM_Free,0.f);
			ConstraintComponent->SetAngularSwing2Limit(ACM_Limited,5.f);
			ConstraintComponent->SetAngularTwistLimit(ACM_Limited,1.f);

			OnTrailerAttach.Broadcast(true,FromSave,ConstraintComponent);
			OtherAttachPoint->OnTrailerAttach.Broadcast(true, FromSave, ConstraintComponent);
			
			
		}
	}
}
