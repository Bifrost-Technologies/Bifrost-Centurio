// Copyright Bifrost-Centurio2026. Licensed under Apache-2.0.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CenturioOpNavCamera.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UCenturioBridgeSubsystem;

/**
 * Placeable OpNav camera for a simulated spacecraft.
 *
 * Attach this actor to your spacecraft pawn/actor in a solar-system level.
 * On a timer (or on demand via CaptureNow) it renders the scene through a
 * SceneCapture2D, saves a PNG, and writes the GIANT metadata sidecar JSON
 * (tools/giant_integration schema) using the latest nav state received from
 * the flight software through UCenturioBridgeSubsystem.
 *
 * The ground system ingests the output directory with
 * tools/giant_integration/ue5_to_giant.py to estimate the spacecraft position
 * from the imagery.
 */
UCLASS(Blueprintable, ClassGroup = (Centurio), meta = (DisplayName = "Centurio OpNav Camera"))
class CENTURIOBRIDGE_API ACenturioOpNavCamera : public AActor
{
    GENERATED_BODY()

public:
    ACenturioOpNavCamera();

    /** Capture a single OpNav image immediately. Returns the image path on success. */
    UFUNCTION(BlueprintCallable, Category = "Centurio|OpNav")
    FString CaptureNow();

    /** Start/stop the periodic capture timer at runtime */
    UFUNCTION(BlueprintCallable, Category = "Centurio|OpNav")
    void SetAutoCapture(bool bEnabled);

    // --- configuration ---

    /** Capture automatically every CaptureIntervalSeconds while playing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Centurio|OpNav")
    bool bAutoCapture = true;

    /** Seconds between automatic captures */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Centurio|OpNav", meta = (ClampMin = "0.05"))
    float CaptureIntervalSeconds = 1.0f;

    /** Rendered image width in pixels */
    UPROPERTY(EditAnywhere, Category = "Centurio|OpNav", meta = (ClampMin = "64", ClampMax = "8192"))
    int32 ImageWidth = 1024;

    /** Rendered image height in pixels */
    UPROPERTY(EditAnywhere, Category = "Centurio|OpNav", meta = (ClampMin = "64", ClampMax = "8192"))
    int32 ImageHeight = 1024;

    /** Horizontal field of view in degrees (recorded in the sidecar for the GIANT camera model) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Centurio|OpNav", meta = (ClampMin = "1.0", ClampMax = "170.0"))
    float FieldOfViewDeg = 90.0f;

    /** Simulated exposure time recorded in the sidecar (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Centurio|OpNav", meta = (ClampMin = "0.0"))
    float ExposureSeconds = 0.01f;

    /**
     * Output directory for PNG + JSON pairs.  Empty = <ProjectSaved>/CenturioOpNav.
     * Point this at the directory your ground system ingests (a network share works).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Centurio|OpNav")
    FString OutputDirectory;

    /** Optional observation target name recorded in the sidecar (e.g. "EARTH", "MOON") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Centurio|OpNav")
    FString TargetName;

    /** Skip captures (with a warning) when no nav telemetry has arrived recently */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Centurio|OpNav")
    bool bRequireNavLink = true;

    /** The scene capture component performing the rendering */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Centurio|OpNav")
    TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void HandleCaptureTimer();
    FString ResolvedOutputDirectory() const;
    bool WriteSidecar(const FString& ImagePath, const FDateTime& CaptureTimeUtc) const;
    UCenturioBridgeSubsystem* GetBridge() const;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> RenderTarget;

    FTimerHandle CaptureTimerHandle;
    int32 CaptureIndex = 0;
};
