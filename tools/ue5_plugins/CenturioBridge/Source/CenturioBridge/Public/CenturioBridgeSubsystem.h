// Copyright Bifrost-Centurio2026. Licensed under Apache-2.0.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CenturioNavTypes.h"
#include "CenturioBridgeSubsystem.generated.h"

class FSocket;
class FUdpSocketReceiver;
class FInternetAddr;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCenturioNavUpdated, const FCenturioNavState&, NavState);

/**
 * UDP link to the cFS ue5_bridge app.
 *
 * Listens for {"type":"nav", ...} JSON datagrams from the flight software
 * (bridge TX port, default 15002) and can send JSON commands back to the
 * bridge RX port (default 15001).  Auto-starts with the game instance so a
 * level only needs the plugin enabled to be connected.
 */
UCLASS(Config = Game)
class CENTURIOBRIDGE_API UCenturioBridgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Latest nav state received from the flight software */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Centurio|Bridge")
    const FCenturioNavState& GetNavState() const { return NavState; }

    /** True if a nav datagram arrived within the last TimeoutSeconds */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Centurio|Bridge")
    bool IsLinkAlive(float TimeoutSeconds = 5.0f) const;

    /** Fired on the game thread whenever a nav datagram is received */
    UPROPERTY(BlueprintAssignable, Category = "Centurio|Bridge")
    FOnCenturioNavUpdated OnNavUpdated;

    // --- commands to the flight software (via bridge RX port) ---

    /** Send a raw JSON command datagram to the bridge */
    UFUNCTION(BlueprintCallable, Category = "Centurio|Bridge")
    bool SendJsonCommand(const FString& Json);

    /** 1=SAFE, 2=GUIDANCE, 3=MANUAL */
    UFUNCTION(BlueprintCallable, Category = "Centurio|Bridge")
    bool SendSetMode(uint8 Mode);

    UFUNCTION(BlueprintCallable, Category = "Centurio|Bridge")
    bool SendSetThrottle(float Percent);

    UFUNCTION(BlueprintCallable, Category = "Centurio|Bridge")
    bool SendSetTargetPosition(double LatDeg, double LonDeg, float AltM);

    UFUNCTION(BlueprintCallable, Category = "Centurio|Bridge")
    bool SendSetTargetVelocity(float VNorth, float VEast, float VDown);

    UFUNCTION(BlueprintCallable, Category = "Centurio|Bridge")
    bool SendSetTargetAttitude(float YawDeg, float PitchDeg, float RollDeg);

    // --- configuration (override in DefaultGame.ini, [/Script/CenturioBridge.CenturioBridgeSubsystem]) ---

    /** UDP port this plugin listens on for nav JSON (bridge UE5_BRIDGE_JSON_TX_PORT) */
    UPROPERTY(Config, EditAnywhere, Category = "Centurio|Bridge")
    int32 NavListenPort = 15002;

    /** Host running the cFS ue5_bridge app */
    UPROPERTY(Config, EditAnywhere, Category = "Centurio|Bridge")
    FString BridgeHost = TEXT("127.0.0.1");

    /** UDP port the bridge listens on for commands (bridge UE5_BRIDGE_JSON_RX_PORT) */
    UPROPERTY(Config, EditAnywhere, Category = "Centurio|Bridge")
    int32 BridgeCommandPort = 15001;

private:
    void HandleNavDatagram(const TArray<uint8>& Data);

    FSocket* NavSocket = nullptr;
    FSocket* CommandSocket = nullptr;
    FUdpSocketReceiver* NavReceiver = nullptr;
    TSharedPtr<FInternetAddr> BridgeAddr;

    FCenturioNavState NavState;
    double LastNavTimeSeconds = -1.0e9;
};
