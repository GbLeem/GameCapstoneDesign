// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JointController.generated.h"

USTRUCT(BlueprintType)
struct GCD_API FAntagonisticController
{
	GENERATED_BODY()

	FVector GetOutput(FVector LowError, FVector HighError, FVector AngularSpeed, float DeltaTime);

	FVector ComputeRequiredAntagonisticTorque
	(
		FVector MinSoftLimit, FVector MaxSoftLimit,
		FQuat PhysicalLocalOrientation, FQuat KinematicLocalOrientation,
		FPhysicsActorHandle RigidActor, float DeltaTime
	);

	void SetGains(const FJointControllerData& Data);

	float IntegralGain;
	float DerivativeGain;
	float ProportionalGainLow;

	FVector ProportionalGainHigh;
	FVector IntegralError;
};

USTRUCT(BlueprintType)
struct GCD_API FJointControllerData
{
	GENERATED_BODY()

	FJointControllerData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicalAnimation)
	FVector MinSoftLimit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicalAnimation)
	FVector MaxSoftLimit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicalAnimation)
	FVector MinHardLimit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicalAnimation)
	FVector MaxHardLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicalAnimation)
	float IntegralGain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicalAnimation)
	float DerivativeGain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicalAnimation)
	float ProportionalGain;
	
	float StiffnessMultiplyer;
	FName BodyName;
	bool bSimulatePhysics;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GCD_API UJointController : public UActorComponent
{
	GENERATED_BODY()

public:
	UJointController(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent);

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation, meta = (UnsafeDuringActorConstruction))
	void SetStiffnessMultiplyer(FName BodyName, float StiffnessMultiplyer);

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation, meta = (UnsafeDuringActorConstruction))
	void SetStiffnessMultiplyersBelow(FName BodyName, float StiffnessMultiplyer, bool bIncludeSelf = true);

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation, meta = (UnsafeDuringActorConstruction))
	void SetGains(FName BodyName, float Integral, float Derivative, float Proportional);

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation, meta = (UnsafeDuringActorConstruction))
	void SetGainsBelow(FName BodyName, float Integral, float Derivative, float Proportional, bool bIncludeSelf = true);

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation, meta = (UnsafeDuringActorConstruction))
	void SetSimulatePhysicsBelow(FName BodyName, bool bSimulatePhysics, bool bIncludeSelf = true);

	virtual void InitializeComponent() override;
	virtual void BeginDestroy() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdatePhysicsEngine();
	void UpdatePhysicsEngineImp();
	void InitComponent();
	void OnTeleport();
	void UpdateTargetActors(ETeleportType TeleportType, float DeltaTime);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PhysicalAnimation, meta = (ClampMin = "0"))
	float StrengthMultiplyer;

private:
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	struct FJointControllerInstanceData
	{
		FConstraintInstance* ConstraintInstance;
		FPhysicsActorHandle TargetActor;
		FAntagonisticController Controller;
	};

	TArray<FJointControllerInstanceData> RuntimeInstanceData;
	TArray<FJointControllerData> DriveData;

	FDelegateHandle OnTeleportDelegateHandle;

	static const FConstraintProfileProperties PhysicalAnimationProfile;

	bool bPhysicsEngineNeedsUpdating;
};
