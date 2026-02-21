#pragma once
#include "Core/Public/Object.h"
#include "Global/Vector.h"

// FMOD forward declarations at global scope
namespace FMOD
{
    class System;
    class Sound;
    class Channel;
    class ChannelGroup;
}

UCLASS()
class USoundManager :public UObject
{
    GENERATED_BODY()
    DECLARE_SINGLETON_CLASS(USoundManager, UObject)

public:
    // 오디오 시스템 수명주기
    void InitializeAudio();
    void ShutdownAudio();

    // 프레임 업데이트(페이드, 스트리밍 업데이트 등)
    void Update(float DeltaTime);

    // 리스너 속성(3D 오디오)
    void SetListener(const FVector& Position, const FVector& Forward, const FVector& Up, const FVector& Velocity);

    // BGM 제어
    bool PlayBGM(const FString& FilePath, bool bLoop = true, float FadeSeconds = 0.5f);
    void StopBGM(float FadeSeconds = 0.3f);
    void PauseBGM();
    void ResumeBGM();
    void SetBGMVolume(float Volume01);

    // SFX(효과음) 관리
    bool PreloadSFX(const FName& SoundName, const FString& FilePath, bool bIsThreeDimensional = true, float MinDistance = 1.0f, float MaxDistance = 30.0f);
    void UnloadSFX(const FName& SoundName);
    bool PlaySFX(const FName& SoundName, float Volume = 1.0f, float Pitch = 1.0f);
    bool PlaySFXAt(const FName& SoundName, const FVector& Position, const FVector& Velocity, float Volume = 1.0f, float Pitch = 1.0f);

    // 루프 SFX 관리(2D 루프 전용)
    bool PlayLoopingSFX(const FName& SoundName, const FString& FilePath, float Volume = 1.0f);
    void StopLoopingSFX(const FName& SoundName);

    // 전체 사운드 정지 및 정리 (BGM + 모든 SFX 채널/루프)
    void StopAllSounds();

private:
    // 내부 상태와 자원
    FMOD::System* AudioSystem = nullptr;
    FMOD::ChannelGroup* MasterChannelGroup = nullptr;
    FMOD::ChannelGroup* BGMChannelGroup = nullptr;
    FMOD::ChannelGroup* SFXChannelGroup = nullptr;

    // BGM 상태
    FMOD::Sound* CurrentBGMSound = nullptr;
    FMOD::Channel* CurrentBGMChannel = nullptr;
    FString CurrentBGMPath;
    float BGMVolume = 0.8f;

    // SFX 캐시와 활성 채널(간단 관리)
    TMap<FName, FMOD::Sound*> SFXCache;
    TArray<FMOD::Channel*> ActiveSFXChannels;

    // 루프 SFX 관리용(이름 기반)
    TMap<FName, FMOD::Sound*> LoopingSFXSounds;
    TMap<FName, FMOD::Channel*> LoopingSFXChannels;
};
