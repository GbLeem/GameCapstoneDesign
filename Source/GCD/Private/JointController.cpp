// Fill out your copyright notice in the Description page of Project Settings.


#include "JointController.h"
#include "DrawDebugHelpers.h"
#include "Chaos/ChaosConstraintSettings.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"

FVector FAntagonisticController::GetOutput(FVector LowError, FVector HighError, FVector AngularSpeed, float DeltaTime)
{
	IntegralError += LowError * DeltaTime;
	return LowError * ProportionalGainLow + HighError * ProportionalGainHigh + IntegralError * IntegralGain - AngularSpeed * DerivativeGain;
}

FVector ComputeGravityTorqueVector(FPhysicsActorHandle RigidActor)
{
	FTransform ActorTransform = FTransform(RigidActor->GetGameThreadAPI().R(), RigidActor->GetGameThreadAPI().X());

	FVector Distance = RigidActor->GetGameThreadAPI().CenterOfMass();

	FVector GravityTorqueVector = FVector::CrossProduct(Distance, RigidActor->GetGameThreadAPI().M() * FVector(0, 0, -98));
	GravityTorqueVector = ActorTransform.InverseTransformVector(GravityTorqueVector);
	GravityTorqueVector /= FVector(RigidActor->GetGameThreadAPI().I());

	return GravityTorqueVector;
}

FVector FAntagonisticController::ComputeRequiredAntagonisticTorque(FVector MinSoftLimit, FVector MaxSoftLimit, FQuat PhysicalLocalOrientation, FQuat KinematicLocalOrientation, FPhysicsActorHandle RigidActor, float DeltaTime)
{
	FVector PhysicalLocalOrientationAngle = FVector
	(
		PhysicalLocalOrientation.GetTwistAngle(FVector::ForwardVector),
		PhysicalLocalOrientation.GetTwistAngle(FVector::RightVector),
		PhysicalLocalOrientation.GetTwistAngle(FVector::UpVector)
	);

	FVector KinematicLocalOrientationAngle = FVector
	(
		KinematicLocalOrientation.GetTwistAngle(FVector::ForwardVector),
		KinematicLocalOrientation.GetTwistAngle(FVector::RightVector),
		KinematicLocalOrientation.GetTwistAngle(FVector::UpVector)
	);

	PhysicalLocalOrientationAngle = FMath::RadiansToDegrees(PhysicalLocalOrientationAngle);
	KinematicLocalOrientationAngle = FMath::RadiansToDegrees(KinematicLocalOrientationAngle);

	//MinSoftLimit = FMath::DegreesToRadians(MinSoftLimit);
	//MaxSoftLimit = FMath::DegreesToRadians(MaxSoftLimit);

	FVector PhysicalLocalErrorMin = MinSoftLimit - PhysicalLocalOrientationAngle;
	FVector PhysicalLocalErrorMax = MaxSoftLimit - PhysicalLocalOrientationAngle;

	FVector KinematicLocalErrorMin = MinSoftLimit - KinematicLocalOrientationAngle;
	FVector KinematicLocalErrorMax = MaxSoftLimit - KinematicLocalOrientationAngle;

	FVector Intercept = -ComputeGravityTorqueVector(RigidActor) / KinematicLocalErrorMax;
	FVector Slope = KinematicLocalErrorMin / -KinematicLocalErrorMax;

	ProportionalGainHigh = ProportionalGainLow * Slope + Intercept;

	UE_LOG(LogTemp, Log, TEXT("KPL: %f, PhysicalLocalErrorMin: %s, Multiple: %s"), ProportionalGainLow, *PhysicalLocalErrorMin.ToString(), *(ProportionalGainLow * PhysicalLocalErrorMin).ToString());
	UE_LOG(LogTemp, Log, TEXT("KPH: %s, PhysicalLocalErrorMax: %s, Multiple: %s"), *ProportionalGainHigh.ToString(), *PhysicalLocalErrorMax.ToString(), *(ProportionalGainHigh * PhysicalLocalErrorMax).ToString());

	FVector TorqueApplied = GetOutput(PhysicalLocalErrorMin, PhysicalLocalErrorMax, FPhysicsInterface::GetAngularVelocity_AssumesLocked(RigidActor), DeltaTime);

	TorqueApplied = RigidActor->GetGameThreadAPI().RotationOfMass() * TorqueApplied;
	TorqueApplied = TorqueApplied * FPhysicsInterface::GetLocalInertiaTensor_AssumesLocked(RigidActor);
	TorqueApplied = RigidActor->GetGameThreadAPI().RotationOfMass().Inverse() * TorqueApplied;

	return TorqueApplied;
}

void FAntagonisticController::SetGains(const FJointControllerData& Data)
{
	IntegralGain = Data.IntegralGain * Data.StiffnessMultiplyer;
	DerivativeGain = Data.DerivativeGain * Data.StiffnessMultiplyer;
	ProportionalGainLow = Data.ProportionalGain * Data.StiffnessMultiplyer;
}

FJointControllerData::FJointControllerData()
	: MinSoftLimit(-180)
	, MaxSoftLimit(180)
	, MinHardLimit(-180)
	, MaxHardLimit(180)
	, IntegralGain(0)
	, DerivativeGain(0)
	, ProportionalGain(0)
	, StiffnessMultiplyer(1)
	, BodyName(NAME_None)
{

}

UJointController::UJointController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	bPhysicsEngineNeedsUpdating = false;

	StrengthMultiplyer = 1.f;
}

void UJointController::SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
{
	if (SkeletalMeshComponent && OnTeleportDelegateHandle.IsValid())
	{
		SkeletalMeshComponent->UnregisterOnTeleportDelegate(OnTeleportDelegateHandle);
	}

	SkeletalMeshComponent = InSkeletalMeshComponent;
	DriveData.Empty();
	RuntimeInstanceData.Reset();

	InitComponent();
}

void UJointController::SetStiffnessMultiplyer(FName BodyName, float StiffnessMultiplyer)
{
	UPhysicsAsset* PhysAsset = SkeletalMeshComponent ? SkeletalMeshComponent->GetPhysicsAsset() : nullptr;
	if (PhysAsset && SkeletalMeshComponent)
	{
		int32 BodyIdx = PhysAsset->FindBodyIndex(BodyName);
		if (BodyIdx != INDEX_NONE)
		{
			DriveData[BodyIdx].StiffnessMultiplyer = StiffnessMultiplyer;
			UpdatePhysicsEngine();
		}
	}
}

void UJointController::SetStiffnessMultiplyersBelow(FName BodyName, float StiffnessMultiplyer, bool bIncludeSelf)
{
	UPhysicsAsset* PhysAsset = SkeletalMeshComponent ? SkeletalMeshComponent->GetPhysicsAsset() : nullptr;
	if (PhysAsset && SkeletalMeshComponent) 
	{
		int NumSetGainsCount = SkeletalMeshComponent->ForEachBodyBelow(BodyName, bIncludeSelf, false, [&](const FBodyInstance* BI)
		{
			int BodyIdx = BI->InstanceBodyIndex;
			DriveData[BodyIdx].StiffnessMultiplyer = StiffnessMultiplyer;
		});

		if (NumSetGainsCount > 0)
		{
			UpdatePhysicsEngine();
		}
	}
}

void UJointController::SetGains(FName BodyName, float Integral, float Derivative, float Proportional)
{
	UPhysicsAsset* PhysAsset = SkeletalMeshComponent ? SkeletalMeshComponent->GetPhysicsAsset() : nullptr;
	if (PhysAsset && SkeletalMeshComponent)
	{
		int32 BodyIdx = PhysAsset->FindBodyIndex(BodyName);
		if (BodyIdx != INDEX_NONE)
		{
			DriveData[BodyIdx].IntegralGain = Integral;
			DriveData[BodyIdx].DerivativeGain = Derivative;
			DriveData[BodyIdx].ProportionalGain = Proportional;
			UpdatePhysicsEngine();
		}
	}
}

void UJointController::SetGainsBelow(FName BodyName, float Integral, float Derivative, float Proportional, bool bIncludeSelf)
{
	UPhysicsAsset* PhysAsset = SkeletalMeshComponent ? SkeletalMeshComponent->GetPhysicsAsset() : nullptr;
	if (PhysAsset && SkeletalMeshComponent)
	{
		int NumSetGainsCount = SkeletalMeshComponent->ForEachBodyBelow(BodyName, bIncludeSelf, false, [&](const FBodyInstance* BI)
		{
			int BodyIdx = BI->InstanceBodyIndex;
			DriveData[BodyIdx].IntegralGain = Integral;
			DriveData[BodyIdx].DerivativeGain = Derivative;
			DriveData[BodyIdx].ProportionalGain = Proportional;
		});

		if (NumSetGainsCount > 0)
		{
			UpdatePhysicsEngine();
		}
	}
}

void UJointController::SetSimulatePhysicsBelow(FName BodyName, bool bSimulatePhysics, bool bIncludeSelf)
{
	UPhysicsAsset* PhysAsset = SkeletalMeshComponent ? SkeletalMeshComponent->GetPhysicsAsset() : nullptr;
	if (PhysAsset && SkeletalMeshComponent)
	{
		SkeletalMeshComponent->ForEachBodyBelow(BodyName, bIncludeSelf, false, [&](const FBodyInstance* BI)
		{
			int BodyIdx = BI->InstanceBodyIndex;
			DriveData[BodyIdx].bSimulatePhysics = bSimulatePhysics;
		});
		SkeletalMeshComponent->SetAllBodiesBelowSimulatePhysics(BodyName, bSimulatePhysics);
	}
}

void UJointController::InitializeComponent()
{
	Super::InitializeComponent();
	InitComponent();
}

void UJointController::BeginDestroy()
{
	if (SkeletalMeshComponent && OnTeleportDelegateHandle.IsValid())
	{
		SkeletalMeshComponent->UnregisterOnTeleportDelegate(OnTeleportDelegateHandle);
	}

	Super::BeginDestroy();
}

void UJointController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (bPhysicsEngineNeedsUpdating)
	{
		UpdatePhysicsEngineImp();
	}

	UpdateTargetActors(ETeleportType::None, DeltaTime);
}

void UJointController::UpdatePhysicsEngine()
{
	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->WakeAllRigidBodies();
	}
	bPhysicsEngineNeedsUpdating = true;
}

FTransform ComputeTargetTM(const USkeletalMeshComponent& SkeletalMeshComponent, const TArray<FTransform>& SpaceBases, int32 BoneIndex)
{
	return SpaceBases[BoneIndex] * SkeletalMeshComponent.GetComponentToWorld();
}

void UJointController::UpdatePhysicsEngineImp()
{
	bPhysicsEngineNeedsUpdating = false;
	UPhysicsAsset* PhysAsset = SkeletalMeshComponent ? SkeletalMeshComponent->GetPhysicsAsset() : nullptr;

	if (PhysAsset && SkeletalMeshComponent)
	{
		const int32 NumData = DriveData.Num();
		const int32 NumInstances = RuntimeInstanceData.Num();

		RuntimeInstanceData.AddZeroed(NumData - NumInstances);

		FPhysicsCommand::ExecuteWrite(SkeletalMeshComponent, [&]()
		{
			for (int32 BodyIdx = 0; BodyIdx < DriveData.Num(); ++BodyIdx)
			{
				const FJointControllerData& Data = DriveData[BodyIdx];
				FJointControllerInstanceData& InstanceData = RuntimeInstanceData[BodyIdx];

				if (InstanceData.ConstraintInstance == nullptr)
				{
					InstanceData.ConstraintInstance = SkeletalMeshComponent->GetConstraintByName(Data.BodyName, false).Get();
					InstanceData.TargetActor = SkeletalMeshComponent->Bodies[BodyIdx]->ActorHandle;
				}

				InstanceData.Controller.SetGains(Data);
			}
		});
	}
}

void UJointController::InitComponent()
{
	if (SkeletalMeshComponent)
	{
		OnTeleportDelegateHandle = SkeletalMeshComponent->RegisterOnTeleportDelegate(FOnSkelMeshTeleported::CreateUObject(this, &UJointController::OnTeleport));
		PrimaryComponentTick.AddPrerequisite(SkeletalMeshComponent, SkeletalMeshComponent->PrimaryComponentTick);

		UPhysicsAsset* PhysAsset =SkeletalMeshComponent->GetPhysicsAsset();

		if (PhysAsset)
		{
			int32 NumBodies = SkeletalMeshComponent->Bodies.Num();

			DriveData.AddZeroed(NumBodies);

			for (int32 BodyIdx = 0; BodyIdx < NumBodies; ++BodyIdx)
			{
				DriveData[BodyIdx].BodyName = PhysAsset->SkeletalBodySetups[BodyIdx]->BoneName;
			}
		}

		UpdatePhysicsEngine();
	}
}

void UJointController::OnTeleport()
{
	if (bPhysicsEngineNeedsUpdating)
	{
		UpdatePhysicsEngineImp();
	}

	UpdateTargetActors(ETeleportType::TeleportPhysics, 0);
}

void UJointController::UpdateTargetActors(ETeleportType TeleportType, float DeltaTime)
{
	UPhysicsAsset* PhysAsset = SkeletalMeshComponent ? SkeletalMeshComponent->GetPhysicsAsset() : nullptr;

	if (PhysAsset && SkeletalMeshComponent->SkeletalMesh)
	{
		const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->SkeletalMesh->GetRefSkeleton();
		const TArray<FTransform>& SpaceBases = SkeletalMeshComponent->GetEditableComponentSpaceTransforms();

		FPhysicsCommand::ExecuteWrite(SkeletalMeshComponent, [&]()
		{
			for (int32 DataIdx = 0; DataIdx < DriveData.Num(); ++DataIdx)
			{
				const FJointControllerData& PhysAnimData = DriveData[DataIdx];
				FJointControllerInstanceData& InstanceData = RuntimeInstanceData[DataIdx];

				if (FPhysicsActorHandle TargetActor = InstanceData.TargetActor)
				{
					const int32 BoneIdx = RefSkeleton.FindBoneIndex(PhysAnimData.BodyName);

					if (BoneIdx != INDEX_NONE && PhysAnimData.bSimulatePhysics)
					{
						const FTransform TargetTM = ComputeTargetTM(*SkeletalMeshComponent, SpaceBases, BoneIdx);

						auto& Constraint = InstanceData.ConstraintInstance;
						FTransform ActorTransform = FPhysicsInterface::GetGlobalPose_AssumesLocked(TargetActor);

						DrawDebugPoint(GetWorld(), TargetTM.GetLocation(), 10, FColor::Black, false, DeltaTime * 2);

						FVector ApplyTorque = InstanceData.Controller.ComputeRequiredAntagonisticTorque(PhysAnimData.MinSoftLimit, PhysAnimData.MaxSoftLimit, ActorTransform.GetRotation(), TargetTM.GetRotation(), InstanceData.TargetActor, DeltaTime);
						UE_LOG(LogTemp, Log, TEXT("BodyName: %s, Torque: %s"), *PhysAnimData.BodyName.ToString(), *ApplyTorque.ToString());
							
						FPhysicsInterface::AddAngularImpulseInRadians_AssumesLocked(TargetActor, ApplyTorque);

						if (TeleportType == ETeleportType::TeleportPhysics)
						{
							FPhysicsInterface::SetGlobalPose_AssumesLocked(TargetActor, TargetTM);
						}
					}
				}
			}
		});
	}
}