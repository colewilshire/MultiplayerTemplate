// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiplayerTemplateGameInstance.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

UMultiplayerTemplateGameInstance::UMultiplayerTemplateGameInstance(const FObjectInitializer& ObjectInitializer)
{

}

void UMultiplayerTemplateGameInstance::Init()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found subsytem %s"), *Subsystem->GetSubsystemName().ToString());

		//IOnlineSessionPtr is a shared pointer. Shared pointers are checked using .IsValid() rather than != nullptr.
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			//Create delegates for create, destroy, and find session
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UMultiplayerTemplateGameInstance::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UMultiplayerTemplateGameInstance::OnDestroySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UMultiplayerTemplateGameInstance::OnFindSessionsComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UMultiplayerTemplateGameInstance::OnJoinSessionComplete);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Found null subsytem"));
	}

	//Handler for network failure (i.e. the host leaves the server)
	if (GEngine != nullptr)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UMultiplayerTemplateGameInstance::OnNetworkFailure);
	}
}

void UMultiplayerTemplateGameInstance::Host(FString ServerName)
{
	DesiredServerName = ServerName;
	if (SessionInterface.IsValid())
	{
		FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SESSION_NAME);
		if (ExistingSession != nullptr)
		{
			SessionInterface->DestroySession(SESSION_NAME);
		}
		else
		{
			CreateSession();
		}
	}
}

void UMultiplayerTemplateGameInstance::Join(uint32 Index)
{
	if (!SessionInterface.IsValid()) return;
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
		UE_LOG(LogTemp, Warning, TEXT("Starting find sessions"));
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
	SteamFriends()->ActivateGameOverlay("Friends");
}

void UMultiplayerTemplateGameInstance::OpenInviteFriendsDialog()
{
	if (CurrentLobbyId.IsValid())
	{
		SteamFriends()->ActivateGameOverlayInviteDialog(CurrentLobbyId);
	}
}

void UMultiplayerTemplateGameInstance::CreateSession()
{
	if (SessionInterface.IsValid())
	{
		FOnlineSessionSettings SessionSettings;

		//If subsytem is null (Steam is not being used), enable LAN.
		if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
		{
			SessionSettings.bIsLANMatch = true;
		}
		//If Steam subsytem is enabled, disable LAN
		else
		{
			SessionSettings.bIsLANMatch = false;
		}
		SessionSettings.bUseLobbiesIfAvailable = true;
		SessionSettings.NumPublicConnections = 5;	//Number of max players
		SessionSettings.bShouldAdvertise = true;
		SessionSettings.bUsesPresence = true;	//Enables Steam lobby
		SessionSettings.Set(SERVER_NAME_SETTINGS_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		SessionInterface->CreateSession(0, SESSION_NAME, SessionSettings);
	}
}

void UMultiplayerTemplateGameInstance::OnCreateSessionComplete(FName SessionName, bool Success)
{
	if (!Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not create session"));
		return;
	}

	//On-screen debug message
	UEngine* Engine = GetEngine();	//Get engine pointer
	if (!ensure(Engine != nullptr)) return;	//Protection from engine pointer being null causing editor crash

	Engine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Green, TEXT("Hosting"));

	//Server Travel (All player controllers in the server move to another level on the server. Creates a server if one does not already exist.)
	UWorld* World = GetWorld();	//Get world object
	if (!ensure(World != nullptr)) return;	//Protection from world pointer being null causing editor crash

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
	if (!SessionInterface.IsValid()) return;

	FString Address;
	if (!SessionInterface->GetResolvedConnectString(SessionName, Address))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not get connect string."));
		return;
	}

	//On-screen debug message
	UEngine* Engine = GetEngine();	//Get engine object
	if (!ensure(Engine != nullptr)) return;	//Protection from engine pointer being null causing crash

	Engine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Green, FString::Printf(TEXT("Joining %s"), *Address));

	//Client travel (client's player controller moves to a server being hosted)
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!ensure(PlayerController != nullptr)) return;	//Protection from player controller pointer being null causing crash

	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}

void UMultiplayerTemplateGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!ensure(PlayerController != nullptr)) return;	//Protection from player controller pointer being null causing crash

	PlayerController->ClientTravel("/Game/TopDown/Maps/TopDownMap", ETravelType::TRAVEL_Absolute);
}

void UMultiplayerTemplateGameInstance::OnGameOverlayActivated(GameOverlayActivated_t* pCallback)
{
	if (pCallback->m_bActive)
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Red, TEXT("Overlay Active"));
	else
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Blue, TEXT("Overlay Inactive"));
}

void UMultiplayerTemplateGameInstance::OnLobbyEntered(LobbyEnter_t* pCallback)
{
	uint64 LobbyId = pCallback->m_ulSteamIDLobby;

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Yellow, FString::FromInt(LobbyId));
	CurrentLobbyId.SetFromUint64(LobbyId);	//This probably needs to be unset when leaving a lobby
}

void UMultiplayerTemplateGameInstance::OnLobbyDataUpdated(LobbyDataUpdate_t* pCallback)
{
	uint64 LobbyId = pCallback->m_ulSteamIDLobby;

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Orange, FString::FromInt(LobbyId));
}
