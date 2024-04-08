// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "AlienManager.generated.h"


USTRUCT(BlueprintType)
struct FAlienList : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	FAlienList() {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlienData")
	TSoftObjectPtr<USkeletalMesh> alienMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlienData")
	int alienAnimBP;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlienData")
	TSoftObjectPtr<UDataTable> alienData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlienData")
	FString startingOutput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlienData")
	int startingAnimation = 0;
	
};

USTRUCT(BlueprintType)
struct FAlienData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	FAlienData() {}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	TArray<FString> correctInputs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	FString correctOutput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	TArray<FString> specialInputs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	TArray<FString> specialOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	FString defaultWrongOutput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactions")
	int correctAnim = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactions")
	TArray<int> specialAnim;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactions")
	int wrongAnim = 0;
};
	
UCLASS()
class BACHELORPROJECT_API AAlienManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAlienManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BluePrintReadWrite)
	FAlienList aliens;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	FAlienData alienData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	int phaseCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	int maxPhases;
	
	UFUNCTION(BlueprintCallable, Meta = (DefaultToSelf))
	void SetAlien(FAlienList aliensIn);

	UFUNCTION(BlueprintCallable, Meta = (DefaultToSelf))
	void SetupLevelPhase(FAlienData alienDataIn);

	UFUNCTION(BlueprintCallable, Meta = (DefaultToSelf))
	FString ManageAlienResponse(FString playerInput);
	
	UPROPERTY(BluePrintReadWrite)
	FString alienTextOutput;

	UPROPERTY(BlueprintReadOnly)
	FString lowerAlphabet = "abcdefghijklmnopqrstuvwxyz";
	UPROPERTY(BlueprintReadOnly)
	FString upperAlphabet = lowerAlphabet.ToUpper();
	
	UFUNCTION(BlueprintCallable, Meta = (DefaultToSelf))
	FText ExcludeCharacters(FText text);

	UFUNCTION(BlueprintCallable, Meta = (DefaultToSelf))
	TArray<FString> SplitOutput(FString text);
	
	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	class AAlienActor* currentAlien;
	AActor* alienClone;

	UPROPERTY(BlueprintReadWrite)
	bool phaseChange = false;
	
	UPROPERTY(BlueprintReadWrite)
	bool levelDone = false;

	UPROPERTY(BlueprintReadWrite)
	int AnimToPlay = 0;

	UPROPERTY(BlueprintReadWrite)
	int CurrentSpecial = 0;
};
