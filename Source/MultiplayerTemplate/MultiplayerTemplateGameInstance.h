// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "steam/steam_api.h"

#include "MultiplayerTemplateGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERTEMPLATE_API UMultiplayerTemplateGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	CSteamID CurrentLobbyId;

	UMultiplayerTemplateGameInstance(const FObjectInitializer& ObjectInitializer);
	virtual void Init();

	UFUNCTION(Exec)
	void Host(FString ServerName);
	UFUNCTION(Exec)
	void Join(uint32 Index);
	UFUNCTION(Exec)
	void RefreshServerList();
	void StartSession();
	UFUNCTION(Exec)
	void OpenSteamOverlay();
	UFUNCTION(Exec)
	void OpenInviteFriendsDialog();

private:
	IOnlineSessionPtr SessionInterface;
	FString DesiredServerName;
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;
	FName SESSION_NAME = NAME_GameSession;	//NAME_GameSession is an Unreal enum for session name which will work across any version of Unreal
	FName SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");

	void CreateSession();

	//Delegates
	void OnCreateSessionComplete(FName SessionName, bool Success);
	void OnDestroySessionComplete(FName SessionName, bool Success);
	void OnFindSessionsComplete(bool Success);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	STEAM_CALLBACK(UMultiplayerTemplateGameInstance, OnGameOverlayActivated, GameOverlayActivated_t);
	STEAM_CALLBACK(UMultiplayerTemplateGameInstance, OnLobbyEntered, LobbyEnter_t);
	STEAM_CALLBACK(UMultiplayerTemplateGameInstance, OnLobbyDataUpdated, LobbyDataUpdate_t);
};
