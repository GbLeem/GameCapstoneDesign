// Fill out your copyright notice in the Description page of Project Settings.


#include "TestClass.h"
#include "DrawDebugHelpers.h"

UTestClass::UTestClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTestClass::SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
{
	SkeletalMeshComponent = InSkeletalMeshComponent;
	PrimaryComponentTick.AddPrerequisite(SkeletalMeshComponent, SkeletalMeshComponent->PrimaryComponentTick);
}

FTransform ComputeLocalSpaceTransform(const USkeletalMeshComponent& SkeletalMeshComponent, const UPhysicsAsset& PhysAsset, const TArray<FTransform>& LocalTransforms, int32 BoneIndex)
{
	const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent.SkeletalMesh->GetRefSkeleton();
	FTransform AccumulatedDelta = LocalTransforms[BoneIndex];
	int32 CurBoneIdx = BoneIndex;
	while ((CurBoneIdx = RefSkeleton.GetParentIndex(CurBoneIdx)) != INDEX_NONE)
	{
		FName BoneName = RefSkeleton.GetBoneName(CurBoneIdx);
		int32 BodyIndex = PhysAsset.FindBodyIndex(BoneName);

		if (CurBoneIdx == BoneIndex)	//some kind of loop so just stop TODO: warn?
		{
			break;
		}

		if (SkeletalMeshComponent.Bodies.IsValidIndex(BodyIndex))
		{
			if (BodyIndex < SkeletalMeshComponent.Bodies.Num())
			{
				FBodyInstance* ParentBody = SkeletalMeshComponent.Bodies[BodyIndex];
				const FTransform NewWorldTM = AccumulatedDelta * ParentBody->GetUnrealWorldTransform_AssumesLocked();
				return NewWorldTM;
			}
			else
			{
				// Bodies array has changed on us?
				break;
			}
		}

		AccumulatedDelta = AccumulatedDelta * LocalTransforms[CurBoneIdx];
	}

	return FTransform::Identity;
}

void UTestClass::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}