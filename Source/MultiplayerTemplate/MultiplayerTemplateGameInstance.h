// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerTemplateGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERTEMPLATE_API UMultiplayerTemplateGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UMultiplayerTemplateGameInstance(const FObjectInitializer& ObjectInitializer);
	virtual void Init();

	UFUNCTION(Exec)
	void Host(FString ServerName);
	UFUNCTION(Exec)
	void Join(uint32 Index);
	UFUNCTION(Exec)
	void RefreshServerList();
	void StartSession();

private:
	IOnlineSessionPtr SessionInterface;
	FString DesiredServerName;
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	void CreateSession();

	//Delegates
	void OnCreateSessionComplete(FName SessionName, bool Success);
	void OnDestroySessionComplete(FName SessionName, bool Success);
	void OnFindSessionsComplete(bool Success);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
};
