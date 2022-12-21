// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	void JoinFriend(const FOnlineSessionSearchResult& InviteResult);
	UFUNCTION(Exec)
	void RefreshServerList();
	void StartSession();
	UFUNCTION(Exec)
	void OpenSteamOverlay();
	UFUNCTION(Exec)
	void OpenInviteFriendsDialog();

private:
	const FName SESSION_NAME = NAME_GameSession;	//NAME_GameSession is an Unreal enum for session name which will work across any version of Unreal
	const FName SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");
	IOnlineSubsystem* OnlineSubsystem;
	IOnlineSessionPtr SessionInterface;
	IOnlineFriendsPtr FriendsInterface;
	IOnlineExternalUIPtr ExternalUIInterface;
	FString DesiredServerName;
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	void CreateSession();

	//Delegates
	void OnCreateSessionComplete(FName SessionName, bool Success);
	void OnDestroySessionComplete(FName SessionName, bool Success);
	void OnFindSessionsComplete(bool Success);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnSessionUserInviteAccepted(bool Success, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	//Debug
	UFUNCTION(EXEC)
	void PrintDebugScript();
};
