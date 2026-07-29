// Copyright Bifrost-Centurio2026. Licensed under Apache-2.0.

#pragma once

#include "CoreMinimal.h"
#include "CenturioNavTypes.generated.h"

/**
 * Navigation state mirrored from the centurio_nav flight app.
 *
 * Matches the {"type":"nav", ...} JSON datagram emitted by the cFS ue5_bridge
 * app (see apps/ue5_bridge/fsw/src/ue5_bridge_app.c).
 */
USTRUCT(BlueprintType)
struct FCenturioNavState
{
    GENERATED_BODY()

    /** Geodetic latitude in degrees (WGS84) */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    double LatitudeDeg = 0.0;

    /** Geodetic longitude in degrees (WGS84) */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    double LongitudeDeg = 0.0;

    /** Altitude above the ellipsoid in meters */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    float AltitudeM = 0.0f;

    /** NED velocity in m/s */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    float VelNorthMS = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    float VelEastMS = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    float VelDownMS = 0.0f;

    /** Body attitude relative to local NED, degrees (ZYX yaw-pitch-roll) */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    float YawDeg = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    float PitchDeg = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    float RollDeg = 0.0f;

    /** Throttle percent commanded by the ground */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    float ThrottlePercent = 0.0f;

    /** 0=INIT,1=SAFE,2=GUIDANCE,3=MANUAL */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    uint8 SystemStatus = 0;

    /** 0=NONE,2=2D,3=3D */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    uint8 NavFixType = 0;

    /** UTC wall-clock time the datagram was received by the plugin */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    FDateTime ReceivedUtc;

    /** True once at least one nav datagram has been received */
    UPROPERTY(BlueprintReadOnly, Category = "Centurio")
    bool bValid = false;
};
