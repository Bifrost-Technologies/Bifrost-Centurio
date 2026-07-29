// Copyright Bifrost-Centurio2026. Licensed under Apache-2.0.

#include "CenturioOpNavCamera.h"

#include "CenturioBridgeSubsystem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/PlatformFileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TextureResource.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCenturioOpNav, Log, All);

ACenturioOpNavCamera::ACenturioOpNavCamera()
{
    PrimaryActorTick.bCanEverTick = false;

    CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
    RootComponent = CaptureComponent;

    // render on demand only; we drive captures from the timer
    CaptureComponent->bCaptureEveryFrame = false;
    CaptureComponent->bCaptureOnMovement = false;
    CaptureComponent->CaptureSource = SCS_FinalColorLDR;
}

void ACenturioOpNavCamera::BeginPlay()
{
    Super::BeginPlay();

    RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("OpNavRenderTarget"));
    RenderTarget->RenderTargetFormat = RTF_RGBA8;
    RenderTarget->InitAutoFormat(ImageWidth, ImageHeight);
    RenderTarget->UpdateResourceImmediate(true);

    CaptureComponent->TextureTarget = RenderTarget;
    CaptureComponent->FOVAngle = FieldOfViewDeg;

    if (bAutoCapture)
    {
        SetAutoCapture(true);
    }
}

void ACenturioOpNavCamera::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CaptureTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void ACenturioOpNavCamera::SetAutoCapture(bool bEnabled)
{
    bAutoCapture = bEnabled;

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FTimerManager& Timers = World->GetTimerManager();
    Timers.ClearTimer(CaptureTimerHandle);
    if (bEnabled)
    {
        Timers.SetTimer(CaptureTimerHandle, this, &ACenturioOpNavCamera::HandleCaptureTimer,
                        FMath::Max(CaptureIntervalSeconds, 0.05f), /*bLoop=*/true);
    }
}

void ACenturioOpNavCamera::HandleCaptureTimer()
{
    CaptureNow();
}

UCenturioBridgeSubsystem* ACenturioOpNavCamera::GetBridge() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UCenturioBridgeSubsystem>();
        }
    }
    return nullptr;
}

FString ACenturioOpNavCamera::ResolvedOutputDirectory() const
{
    FString Dir = OutputDirectory.IsEmpty()
                      ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CenturioOpNav"))
                      : OutputDirectory;
    FPaths::NormalizeDirectoryName(Dir);
    return Dir;
}

FString ACenturioOpNavCamera::CaptureNow()
{
    UCenturioBridgeSubsystem* Bridge = GetBridge();

    if (bRequireNavLink && (!Bridge || !Bridge->IsLinkAlive()))
    {
        UE_LOG(LogCenturioOpNav, Warning,
               TEXT("Skipping capture: no recent nav telemetry from the flight software"));
        return FString();
    }

    if (!RenderTarget || !CaptureComponent)
    {
        UE_LOG(LogCenturioOpNav, Error, TEXT("Capture components not initialized"));
        return FString();
    }

    // render the scene now
    CaptureComponent->FOVAngle = FieldOfViewDeg;
    CaptureComponent->CaptureScene();

    const FDateTime CaptureTimeUtc = FDateTime::UtcNow();

    // read back the pixels (synchronous; sized for low-rate OpNav captures)
    FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
    if (!Resource)
    {
        UE_LOG(LogCenturioOpNav, Error, TEXT("Render target has no resource"));
        return FString();
    }

    TArray<FColor> Pixels;
    if (!Resource->ReadPixels(Pixels) || Pixels.Num() != ImageWidth * ImageHeight)
    {
        UE_LOG(LogCenturioOpNav, Error, TEXT("ReadPixels failed"));
        return FString();
    }

    // force opaque alpha so the PNG is a clean illumination image
    for (FColor& Pixel : Pixels)
    {
        Pixel.A = 255;
    }

    // compress to PNG
    IImageWrapperModule& ImageWrapperModule =
        FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
    TSharedPtr<IImageWrapper> Png = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
    if (!Png.IsValid() ||
        !Png->SetRaw(Pixels.GetData(), Pixels.Num() * sizeof(FColor), ImageWidth, ImageHeight,
                     ERGBFormat::BGRA, 8))
    {
        UE_LOG(LogCenturioOpNav, Error, TEXT("PNG compression failed"));
        return FString();
    }
    const TArray64<uint8> PngData = Png->GetCompressed();

    // write image + sidecar
    const FString Dir = ResolvedOutputDirectory();
    FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Dir);

    const FString BaseName = FString::Printf(TEXT("opnav_%s_%04d"),
                                             *CaptureTimeUtc.ToString(TEXT("%Y%m%d_%H%M%S")),
                                             CaptureIndex++);
    const FString ImagePath = FPaths::Combine(Dir, BaseName + TEXT(".png"));

    if (!FFileHelper::SaveArrayToFile(PngData, *ImagePath))
    {
        UE_LOG(LogCenturioOpNav, Error, TEXT("Failed to write %s"), *ImagePath);
        return FString();
    }

    if (!WriteSidecar(ImagePath, CaptureTimeUtc))
    {
        UE_LOG(LogCenturioOpNav, Warning,
               TEXT("Image saved without sidecar (no nav state); GIANT ingest will skip it: %s"),
               *ImagePath);
    }
    else
    {
        UE_LOG(LogCenturioOpNav, Log, TEXT("OpNav capture: %s"), *ImagePath);
    }

    return ImagePath;
}

bool ACenturioOpNavCamera::WriteSidecar(const FString& ImagePath, const FDateTime& CaptureTimeUtc) const
{
    const UCenturioBridgeSubsystem* Bridge = GetBridge();
    if (!Bridge || !Bridge->GetNavState().bValid)
    {
        return false;
    }
    const FCenturioNavState& Nav = Bridge->GetNavState();

    // schema: tools/giant_integration/ue5_to_giant.py (write_sidecar)
    TSharedRef<FJsonObject> Meta = MakeShared<FJsonObject>();
    Meta->SetStringField(TEXT("type"), TEXT("opnav_image"));
    Meta->SetStringField(TEXT("time_utc"), CaptureTimeUtc.ToIso8601());
    Meta->SetNumberField(TEXT("exposure_s"), ExposureSeconds);
    Meta->SetNumberField(TEXT("fov_deg"), FieldOfViewDeg);
    Meta->SetNumberField(TEXT("lat"), Nav.LatitudeDeg);
    Meta->SetNumberField(TEXT("lon"), Nav.LongitudeDeg);
    Meta->SetNumberField(TEXT("alt_m"), Nav.AltitudeM);
    Meta->SetNumberField(TEXT("vn"), Nav.VelNorthMS);
    Meta->SetNumberField(TEXT("ve"), Nav.VelEastMS);
    Meta->SetNumberField(TEXT("vd"), Nav.VelDownMS);
    Meta->SetNumberField(TEXT("yaw"), Nav.YawDeg);
    Meta->SetNumberField(TEXT("pitch"), Nav.PitchDeg);
    Meta->SetNumberField(TEXT("roll"), Nav.RollDeg);
    if (!TargetName.IsEmpty())
    {
        Meta->SetStringField(TEXT("target"), TargetName);
    }

    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    if (!FJsonSerializer::Serialize(Meta, Writer))
    {
        return false;
    }

    const FString SidecarPath = FPaths::ChangeExtension(ImagePath, TEXT("json"));
    return FFileHelper::SaveStringToFile(Json, *SidecarPath,
                                         FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
