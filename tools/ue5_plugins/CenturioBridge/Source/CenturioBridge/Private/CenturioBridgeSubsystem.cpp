// Copyright Bifrost-Centurio2026. Licensed under Apache-2.0.

#include "CenturioBridgeSubsystem.h"

#include "Async/Async.h"
#include "Common/UdpSocketBuilder.h"
#include "Common/UdpSocketReceiver.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

DEFINE_LOG_CATEGORY_STATIC(LogCenturioBridge, Log, All);

void UCenturioBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogCenturioBridge, Error, TEXT("No socket subsystem available"));
        return;
    }

    // RX: nav telemetry from the flight software
    NavSocket = FUdpSocketBuilder(TEXT("CenturioNavRx"))
                    .AsNonBlocking()
                    .AsReusable()
                    .BoundToAnyAddress()
                    .BoundToPort(NavListenPort)
                    .WithReceiveBufferSize(64 * 1024);

    if (NavSocket)
    {
        NavReceiver = new FUdpSocketReceiver(NavSocket, FTimespan::FromMilliseconds(50), TEXT("CenturioNavReceiver"));
        NavReceiver->OnDataReceived().BindLambda(
            [WeakThis = TWeakObjectPtr<UCenturioBridgeSubsystem>(this)](const FArrayReaderPtr& DataPtr, const FIPv4Endpoint&)
            {
                TArray<uint8> Data(DataPtr->GetData(), DataPtr->Num());
                // marshal to the game thread before touching state / broadcasting
                AsyncTask(ENamedThreads::GameThread, [WeakThis, Data = MoveTemp(Data)]()
                {
                    if (UCenturioBridgeSubsystem* Subsystem = WeakThis.Get())
                    {
                        Subsystem->HandleNavDatagram(Data);
                    }
                });
            });
        NavReceiver->Start();
        UE_LOG(LogCenturioBridge, Log, TEXT("Listening for nav JSON on UDP %d"), NavListenPort);
    }
    else
    {
        UE_LOG(LogCenturioBridge, Error, TEXT("Failed to bind nav RX socket on UDP %d"), NavListenPort);
    }

    // TX: commands to the bridge
    CommandSocket = FUdpSocketBuilder(TEXT("CenturioCmdTx")).AsNonBlocking();

    BridgeAddr = SocketSubsystem->CreateInternetAddr();
    bool bAddrValid = false;
    BridgeAddr->SetIp(*BridgeHost, bAddrValid);
    BridgeAddr->SetPort(BridgeCommandPort);
    if (!bAddrValid)
    {
        UE_LOG(LogCenturioBridge, Error, TEXT("Invalid bridge host '%s'"), *BridgeHost);
    }
    else
    {
        UE_LOG(LogCenturioBridge, Log, TEXT("Bridge command endpoint %s:%d"), *BridgeHost, BridgeCommandPort);
    }
}

void UCenturioBridgeSubsystem::Deinitialize()
{
    if (NavReceiver)
    {
        NavReceiver->Stop();
        delete NavReceiver;
        NavReceiver = nullptr;
    }

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    for (FSocket** Socket : { &NavSocket, &CommandSocket })
    {
        if (*Socket)
        {
            (*Socket)->Close();
            if (SocketSubsystem)
            {
                SocketSubsystem->DestroySocket(*Socket);
            }
            *Socket = nullptr;
        }
    }

    Super::Deinitialize();
}

bool UCenturioBridgeSubsystem::IsLinkAlive(float TimeoutSeconds) const
{
    return NavState.bValid && (FPlatformTime::Seconds() - LastNavTimeSeconds) <= TimeoutSeconds;
}

void UCenturioBridgeSubsystem::HandleNavDatagram(const TArray<uint8>& Data)
{
    // datagram is UTF-8 JSON, not NUL terminated
    FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Data.GetData()), Data.Num());
    const FString Json(Converter.Length(), Converter.Get());

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogCenturioBridge, Verbose, TEXT("Ignoring non-JSON datagram"));
        return;
    }

    FString Type;
    if (!Root->TryGetStringField(TEXT("type"), Type) || Type != TEXT("nav"))
    {
        return;
    }

    NavState.LatitudeDeg     = Root->GetNumberField(TEXT("lat"));
    NavState.LongitudeDeg    = Root->GetNumberField(TEXT("lon"));
    NavState.AltitudeM       = Root->GetNumberField(TEXT("alt"));
    NavState.VelNorthMS      = Root->GetNumberField(TEXT("vn"));
    NavState.VelEastMS       = Root->GetNumberField(TEXT("ve"));
    NavState.VelDownMS       = Root->GetNumberField(TEXT("vd"));
    NavState.YawDeg          = Root->GetNumberField(TEXT("yaw"));
    NavState.PitchDeg        = Root->GetNumberField(TEXT("pitch"));
    NavState.RollDeg         = Root->GetNumberField(TEXT("roll"));
    NavState.ThrottlePercent = Root->GetNumberField(TEXT("thr"));
    NavState.SystemStatus    = static_cast<uint8>(Root->GetIntegerField(TEXT("status")));
    NavState.NavFixType      = static_cast<uint8>(Root->GetIntegerField(TEXT("fix")));
    NavState.ReceivedUtc     = FDateTime::UtcNow();
    NavState.bValid          = true;

    LastNavTimeSeconds = FPlatformTime::Seconds();

    OnNavUpdated.Broadcast(NavState);
}

bool UCenturioBridgeSubsystem::SendJsonCommand(const FString& Json)
{
    if (!CommandSocket || !BridgeAddr.IsValid())
    {
        UE_LOG(LogCenturioBridge, Warning, TEXT("Command socket not available"));
        return false;
    }

    FTCHARToUTF8 Utf8(*Json);
    int32 BytesSent = 0;
    const bool bOk = CommandSocket->SendTo(
        reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), BytesSent, *BridgeAddr);

    if (!bOk || BytesSent != Utf8.Length())
    {
        UE_LOG(LogCenturioBridge, Warning, TEXT("Failed to send command: %s"), *Json);
        return false;
    }
    return true;
}

bool UCenturioBridgeSubsystem::SendSetMode(uint8 Mode)
{
    return SendJsonCommand(FString::Printf(TEXT("{\"type\":\"set_mode\",\"mode\":%u}"), Mode));
}

bool UCenturioBridgeSubsystem::SendSetThrottle(float Percent)
{
    return SendJsonCommand(FString::Printf(TEXT("{\"type\":\"set_throttle\",\"percent\":%.2f}"), Percent));
}

bool UCenturioBridgeSubsystem::SendSetTargetPosition(double LatDeg, double LonDeg, float AltM)
{
    return SendJsonCommand(FString::Printf(
        TEXT("{\"type\":\"set_target_pos\",\"lat\":%.8f,\"lon\":%.8f,\"alt_m\":%.2f}"), LatDeg, LonDeg, AltM));
}

bool UCenturioBridgeSubsystem::SendSetTargetVelocity(float VNorth, float VEast, float VDown)
{
    return SendJsonCommand(FString::Printf(
        TEXT("{\"type\":\"set_target_vel\",\"vn\":%.3f,\"ve\":%.3f,\"vd\":%.3f}"), VNorth, VEast, VDown));
}

bool UCenturioBridgeSubsystem::SendSetTargetAttitude(float YawDeg, float PitchDeg, float RollDeg)
{
    return SendJsonCommand(FString::Printf(
        TEXT("{\"type\":\"set_target_att\",\"yaw\":%.2f,\"pitch\":%.2f,\"roll\":%.2f}"), YawDeg, PitchDeg, RollDeg));
}
