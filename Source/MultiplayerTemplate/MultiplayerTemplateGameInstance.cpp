// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiplayerTemplateGameInstance.h"
#include "Engine/Engine.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"

UMultiplayerTemplateGameInstance::UMultiplayerTemplateGameInstance(const FObjectInitializer& ObjectInitializer)
{

}

void UMultiplayerTemplateGameInstance::Init()
{
	OnlineSubsystem = IOnlineSubsystem::Get();

	//IOnlineSessionPtr is a shared pointer. Shared pointers are checked using .IsValid() rather than != nullptr.
	SessionInterface = OnlineSubsystem->GetSessionInterface();
	FriendsInterface = OnlineSubsystem->GetFriendsInterface();
	ExternalUIInterface = OnlineSubsystem->GetExternalUIInterface();

	//Create delegates for create, destroy, and find session
	SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UMultiplayerTemplateGameInstance::OnCreateSessionComplete);
	SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UMultiplayerTemplateGameInstance::OnDestroySessionComplete);
	SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UMultiplayerTemplateGameInstance::OnFindSessionsComplete);
	SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UMultiplayerTemplateGameInstance::OnJoinSessionComplete);

	//Handler for network failure (i.e. the host leaves the server)
	GEngine->OnNetworkFailure().AddUObject(this, &UMultiplayerTemplateGameInstance::OnNetworkFailure);
}

void UMultiplayerTemplateGameInstance::Host(FString ServerName)
{
	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SESSION_NAME);
	DesiredServerName = ServerName;

	if (ExistingSession != nullptr)
	{
		SessionInterface->DestroySession(SESSION_NAME);
	}
	else
	{
		CreateSession();
	}
}

void UMultiplayerTemplateGameInstance::Join(uint32 Index)
{
	if (!SessionSearch.IsValid()) return;

	SessionInterface->JoinSession(0, SESSION_NAME, SessionSearch->SearchResults[Index]);
}

void UMultiplayerTemplateGameInstance::RefreshServerList()
{
	//Search for sessions
	SessionSearch = MakeShareable(new FOnlineSessionSearch());	//The new keyword means to make something on the heap, instead of the stack
	if (SessionSearch.IsValid())
	{
		SessionSearch->MaxSearchResults = 100;
		SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);	//Allows Steam search presence
		SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	}
}

void UMultiplayerTemplateGameInstance::StartSession()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->StartSession(SESSION_NAME);
	}
}

void UMultiplayerTemplateGameInstance::OpenSteamOverlay()
{
	if (!ExternalUIInterface.IsValid()) return;
	ExternalUIInterface->ShowFriendsUI(0);
}

void UMultiplayerTemplateGameInstance::OpenInviteFriendsDialog()
{
	if (!ExternalUIInterface.IsValid()) return;
	ExternalUIInterface->ShowInviteUI(0);
}

void UMultiplayerTemplateGameInstance::CreateSession()
{
	FOnlineSessionSettings SessionSettings;

	if (OnlineSubsystem->GetSubsystemName() == "NULL")
	{
		//If subsytem is null (Steam is not being used), enable LAN.
		SessionSettings.bIsLANMatch = true;
	}
	else
	{
		//If Steam subsytem is enabled, disable LAN
		SessionSettings.bIsLANMatch = false;
	}
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.NumPublicConnections = 5;	//Number of max players
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = true;	//Enables Steam lobby
	SessionSettings.Set(SERVER_NAME_SETTINGS_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	SessionInterface->CreateSession(0, SESSION_NAME, SessionSettings);
}

void UMultiplayerTemplateGameInstance::OnCreateSessionComplete(FName SessionName, bool Success)
{
	if (!Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not create session"));
		return;
	}

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Green, TEXT("Hosting"));

	//Server Travel (All player controllers in the server move to another level on the server. Creates a server if one does not already exist.)
	UWorld* World = GetWorld();
	World->ServerTravel("/Game/TopDown/Maps/TopDownMap?listen");
}

void UMultiplayerTemplateGameInstance::OnDestroySessionComplete(FName SessionName, bool Success)
{
	if (Success)
	{
		CreateSession();
	}
}

void UMultiplayerTemplateGameInstance::OnFindSessionsComplete(bool Success)
{
	if (Success && SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Finished find session"));

		for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Blue, FString::Printf(TEXT("Found session: %s"), *SearchResult.GetSessionIdStr()));
		}
	}
}

void UMultiplayerTemplateGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	//Get address of multiplayer lobby to connect to
	FString Address;
	if (!SessionInterface->GetResolvedConnectString(SessionName, Address))
	{
		return;
	}

	//On-screen debug message
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Green, FString::Printf(TEXT("Joining %s"), *Address));

	//Client travel (client's player controller moves to a server being hosted)
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}

void UMultiplayerTemplateGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	PlayerController->ClientTravel("/Game/TopDown/Maps/TopDownMap", ETravelType::TRAVEL_Absolute);
}
