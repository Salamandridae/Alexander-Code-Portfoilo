// Fill out your copyright notice in the Description page of Project Settings.


#include "AlienManager.h"
#include "AlienActor.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AAlienManager::AAlienManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAlienManager::BeginPlay()
{
	Super::BeginPlay();

	
}

// Called every frame
void AAlienManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
}

// This is called in the manager blueprint to make sure the data is assigned to the correct alien reference 
void AAlienManager::SetAlien(FAlienList aliensIn)
{
	aliens = aliensIn;
	aliens.alienData = aliensIn.alienData.LoadSynchronous();
	alienClone = UGameplayStatics::GetActorOfClass(GetWorld(), AAlienActor::StaticClass());
	currentAlien = Cast<AAlienActor>(alienClone);
}

// Called in the manager blueprint to assign a data table row for a new phase
// Starts a level's first phase, and its subsequent phases after a correct word has been entered
void AAlienManager::SetupLevelPhase(FAlienData alienDataIn)
{
	alienData = alienDataIn;
	phaseChange = false;
}

// The core function for the gameplay, called in the Gameplay HUD blueprint
FString AAlienManager::ManageAlienResponse(FString playerInput)
{
	playerInput = playerInput.TrimEnd();
	for (int i = 0; i < alienData.correctInputs.Num(); i++)
	{
		if (playerInput.ToUpper().Equals(alienData.correctInputs[i].ToUpper()))
		{
			alienTextOutput = alienData.correctOutput;
			phaseCount++;
			AnimToPlay = 1;
			phaseChange = true;
			return alienTextOutput;
		}
	}
	for (int i = 0; i < alienData.specialInputs.Num(); i++)
	{
		if (playerInput.ToUpper().Equals(alienData.specialInputs[i].ToUpper()))
		{
			alienTextOutput = alienData.specialOutputs[i];
			AnimToPlay = 3;
			CurrentSpecial = i;
			return alienTextOutput;
		}
	}
	alienTextOutput = alienData.defaultWrongOutput;
	AnimToPlay = 2;
	return alienTextOutput;
}

// Function to prevent non-space characters from being typed buy the player.
// Called in Gameplay HUD blueprint on every typing input.
FText AAlienManager::ExcludeCharacters(FText text)
{
	FString newString = "", tempString = text.ToString();
	//Prevents leading spaces and crashing from a negative index
	tempString = tempString.TrimStart();
	for (int i = 0; i < tempString.Len(); i++)
	{	//Prevents consecutive spaces
		if(tempString[i] == ' ' && tempString[i-1] != ' ') 
				newString += tempString[i];
		else //Checks if characters are letters present in the alphabet strings
			for (int j = 0; j < lowerAlphabet.Len(); j++)
				if (lowerAlphabet[j] == tempString[i] || upperAlphabet[j] == tempString[i])
					newString += tempString[i];
	}

	//Capitalising every first letter in the text input
	// if (!newString.IsEmpty())
	// {
	// 	//This check might cause crashes, comment it out or remove it if so.
	// 	newString = newString.ToLower();
	// 	TCHAR c = newString[0];
	// 	FString upperString = " "; upperString[0] = c; upperString = upperString.ToUpper();
	// 	newString[0] = upperString[0];
	// }
	
	//Alternatively, just capitalise every letter.
	newString = newString.ToUpper();
	
	// Returns the reconstructed text to the typing input field 
	return FText::FromString(newString);
}

// Function that splits string into array of strings to highlight the spoken words.
// Called in Gameplay HUD after every alien output.
TArray<FString> AAlienManager::SplitOutput(FString text)
{
	TArray<FString> outputSnippets;
	int32 startIndex = 0, endIndex = 0, snippetLength = text.Len();
	while(text.Contains("\n"))
	{	//Risky c++ while loop, handle code with care. Splitting happens in this loop
		endIndex = text.Find("\n");
		FString snippet = text.Left(endIndex).TrimChar('\r').TrimChar('\n');
		// UE_LOG(LogTemp, Warning, TEXT(": %s"), *snippet);
		snippet.TrimStartAndEnd().Shrink();
		outputSnippets.Add(snippet);
		text = text.RightChop(endIndex).TrimStartAndEnd();
		//UE_LOG(LogTemp, Warning, TEXT(": %s"), *text);
		//Reassurance check to guarantee that loop will always exit. It is actually redundant.
		if(!text.Contains("\n") || !text.Contains("\""))
			break;
	}	// Reducing size and cutting off remaining escape characters,
		// and adding what remains of the input string as the last index
	text.TrimStartAndEnd().Shrink();
	text = text.TrimChar('\r').TrimChar('\n');
		// UE_LOG(LogTemp, Warning, TEXT(": %s"), *text);
	 	outputSnippets.Add(text);

	//Maybe I didn't have to write my own splitting function after all...
	//Is an O(N^2), might be less resource efficient, but works perfectly
	//This is currently called in the HUD blueprint, in UpdateChatLogALien
	//text.ParseIntoArrayLines(outputSnippets, true);
	
	// for (int i = 0; i< outputSnippets.Num(); i++)
	// 	UE_LOG(LogTemp, Warning, TEXT(": %s"), *outputSnippets[i]);
		
	return outputSnippets;
}


