// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TestClass.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GCD_API UTestClass : public UActorComponent
{
	GENERATED_BODY()

	UTestClass(const FObjectInitializer& ObjectInitializer);

public:

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
};