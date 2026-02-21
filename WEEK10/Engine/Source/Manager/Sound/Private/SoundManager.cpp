#include "pch.h"
#include <fmod/fmod.hpp>
#include "Manager/Sound/Public/SoundManager.h"
#include "Global/Macro.h"

IMPLEMENT_SINGLETON_CLASS(USoundManager, UObject)


USoundManager::USoundManager() {}
USoundManager::~USoundManager() = default;

static bool CheckFMODResult(FMOD_RESULT Result, const char* Context)
{
    if (Result != FMOD_OK)
    {
        UE_LOG_ERROR("FMOD Error (%d): %s", (int)Result, Context);
        return false;
    }
    return true;
}

static FMOD_VECTOR ToFMODVector(const FVector& Vector)
{
    FMOD_VECTOR Out; Out.x = Vector.X; Out.y = Vector.Y; Out.z = Vector.Z; return Out;
}

void USoundManager::InitializeAudio()
{
    if (AudioSystem)
        return;

    FMOD_RESULT Result = FMOD::System_Create(&AudioSystem);
    if (!CheckFMODResult(Result, "System_Create")) return;

    Result = AudioSystem->init(512, FMOD_INIT_NORMAL, nullptr);
    if (!CheckFMODResult(Result, "System::init")) return;

    // 채널 그룹 구성
    AudioSystem->createChannelGroup("Master", &MasterChannelGroup);
    AudioSystem->createChannelGroup("BGM", &BGMChannelGroup);
    AudioSystem->createChannelGroup("SFX", &SFXChannelGroup);

    FMOD::ChannelGroup* SystemMaster = nullptr;
    AudioSystem->getMasterChannelGroup(&SystemMaster);
    if (SystemMaster)
    {
        SystemMaster->addGroup(MasterChannelGroup);
        MasterChannelGroup->addGroup(BGMChannelGroup);
        MasterChannelGroup->addGroup(SFXChannelGroup);
    }

    BGMChannelGroup->setVolume(BGMVolume);
    UE_LOG("[Sound] InitializeAudio OK. BGMVolume=%.2f", BGMVolume);
}

void USoundManager::ShutdownAudio()
{
    if (!AudioSystem)
        return;

    // 루프 SFX/채널 등 모든 재생 자원을 먼저 정리
    StopAllSounds();

    if (CurrentBGMChannel)
    {
        CurrentBGMChannel->stop();
        CurrentBGMChannel = nullptr;
    }
    if (CurrentBGMSound)
    {
        CurrentBGMSound->release();
        CurrentBGMSound = nullptr;
    }

    for (auto& Pair : SFXCache)
    {
        if (Pair.second)
        {
            Pair.second->release();
        }
    }
    SFXCache.Empty();

    if (AudioSystem)
    {
        AudioSystem->close();
        AudioSystem->release();
        AudioSystem = nullptr;
    }

    MasterChannelGroup = nullptr;
    BGMChannelGroup = nullptr;
    SFXChannelGroup = nullptr;
}

void USoundManager::Update(float DeltaTime)
{
    if (!AudioSystem) return;
    AudioSystem->update();
}

void USoundManager::SetListener(const FVector& Position, const FVector& Forward, const FVector& Up, const FVector& Velocity)
{
    if (!AudioSystem) return;

    FMOD_VECTOR FmodPosition = ToFMODVector(Position);
    FMOD_VECTOR FmodVelocity = ToFMODVector(Velocity);
    FMOD_VECTOR FmodForward = ToFMODVector(Forward);
    FMOD_VECTOR FmodUp = ToFMODVector(Up);
    AudioSystem->set3DListenerAttributes(0, &FmodPosition, &FmodVelocity, &FmodForward, &FmodUp);
}

bool USoundManager::PlayBGM(const FString& FilePath, bool bLoop, float FadeSeconds)
{
    if (!AudioSystem) return false;
    UE_LOG("[Sound] PlayBGM: %s (loop=%d, fade=%.2f)", FilePath.c_str(), bLoop ? 1 : 0, FadeSeconds);

    if (CurrentBGMPath == FilePath && CurrentBGMChannel)
    {
        bool IsPaused = false;
        CurrentBGMChannel->getPaused(&IsPaused);
        if (IsPaused) CurrentBGMChannel->setPaused(false);
        return true;
    }

    if (CurrentBGMChannel)
    {
        CurrentBGMChannel->stop();
        CurrentBGMChannel = nullptr;
    }
    if (CurrentBGMSound)
    {
        CurrentBGMSound->release();
        CurrentBGMSound = nullptr;
    }

    // BGM should be 2D stream so it is not spatialized/attenuated
    FMOD_MODE Mode = FMOD_CREATESTREAM | FMOD_2D;
    if (bLoop) Mode |= FMOD_LOOP_NORMAL;

    FMOD_RESULT Result = AudioSystem->createSound(FilePath.c_str(), Mode, nullptr, &CurrentBGMSound);
    if (!CheckFMODResult(Result, "CreateSound(BGM)")) { UE_LOG_ERROR("[Sound] CreateSound failed: %s", FilePath.c_str()); return false; }

    FMOD::Channel* NewChannel = nullptr;
    Result = AudioSystem->playSound(CurrentBGMSound, BGMChannelGroup, false, &NewChannel);
    if (!CheckFMODResult(Result, "PlaySound(BGM)")) { UE_LOG_ERROR("[Sound] PlaySound failed"); return false; }

    CurrentBGMChannel = NewChannel;
    CurrentBGMPath = FilePath;
    if (FadeSeconds > 0.0f)
    {
        CurrentBGMChannel->setVolume(0.0f);
        CurrentBGMChannel->setVolume(BGMVolume);
    }
    else
    {
        CurrentBGMChannel->setVolume(BGMVolume);
    }
    bool Paused=false; if (CurrentBGMChannel) CurrentBGMChannel->getPaused(&Paused);
    UE_LOG("[Sound] BGM started. paused=%d", Paused ? 1:0);
    return true;
}

void USoundManager::StopBGM(float FadeSeconds)
{
    if (!CurrentBGMChannel) return;
    CurrentBGMChannel->stop();
    CurrentBGMChannel = nullptr;

    if (CurrentBGMSound)
    {
        CurrentBGMSound->release();
        CurrentBGMSound = nullptr;
    }
    CurrentBGMPath = FString();
}

void USoundManager::PauseBGM()
{
    if (CurrentBGMChannel) CurrentBGMChannel->setPaused(true);
}

void USoundManager::ResumeBGM()
{
    if (CurrentBGMChannel) CurrentBGMChannel->setPaused(false);
}

void USoundManager::SetBGMVolume(float Volume01)
{
    BGMVolume = Volume01;
    if (BGMChannelGroup) BGMChannelGroup->setVolume(BGMVolume);
}

bool USoundManager::PreloadSFX(const FName& SoundName, const FString& FilePath, bool bIsThreeDimensional, float MinDistance, float MaxDistance)
{
    if (!AudioSystem) return false;
    if (SFXCache.Contains(SoundName)) return true;

    FMOD_MODE Mode = FMOD_DEFAULT | FMOD_CREATESAMPLE | (bIsThreeDimensional ? FMOD_3D : FMOD_2D);
    FMOD::Sound* NewSound = nullptr;
    FMOD_RESULT Result = AudioSystem->createSound(FilePath.c_str(), Mode, nullptr, &NewSound);
    if (!CheckFMODResult(Result, "CreateSound(SFX)")) return false;

    if (bIsThreeDimensional && NewSound)
    {
        NewSound->set3DMinMaxDistance(MinDistance, MaxDistance);
    }

    SFXCache.Add(SoundName, NewSound);
    return true;
}

void USoundManager::UnloadSFX(const FName& SoundName)
{
    if (!SFXCache.Contains(SoundName)) return;
    FMOD::Sound** FoundSoundPtr = SFXCache.Find(SoundName);
    FMOD::Sound* TargetSound = FoundSoundPtr ? *FoundSoundPtr : nullptr;
    if (TargetSound)
    {
        TargetSound->release();
    }
    SFXCache.Remove(SoundName);
}

bool USoundManager::PlaySFX(const FName& SoundName, float Volume, float Pitch)
{
    if (!AudioSystem) return false;
    if (!SFXCache.Contains(SoundName)) return false;

    FMOD::Sound** FoundSoundPtr = SFXCache.Find(SoundName);
    FMOD::Sound* TargetSound = FoundSoundPtr ? *FoundSoundPtr : nullptr;
    FMOD::Channel* NewChannel = nullptr;
    FMOD_RESULT Result = AudioSystem->playSound(TargetSound, SFXChannelGroup, false, &NewChannel);
    if (!CheckFMODResult(Result, "PlaySound(SFX)")) return false;

    if (NewChannel)
    {
        NewChannel->setVolume(Volume);
        NewChannel->setPitch(Pitch);
        ActiveSFXChannels.Add(NewChannel);
    }
    return true;
}

bool USoundManager::PlaySFXAt(const FName& SoundName, const FVector& Position, const FVector& Velocity, float Volume, float Pitch)
{
    if (!AudioSystem) return false;
    if (!SFXCache.Contains(SoundName)) return false;

    FMOD::Sound** FoundSoundPtr = SFXCache.Find(SoundName);
    FMOD::Sound* TargetSound = FoundSoundPtr ? *FoundSoundPtr : nullptr;
    FMOD::Channel* NewChannel = nullptr;
    FMOD_RESULT Result = AudioSystem->playSound(TargetSound, SFXChannelGroup, true, &NewChannel);
    if (!CheckFMODResult(Result, "playSound(SFX3D)")) return false;

    if (NewChannel)
    {
        FMOD_VECTOR FmodPosition = ToFMODVector(Position);
        FMOD_VECTOR FmodVelocity = ToFMODVector(Velocity);
        NewChannel->set3DAttributes(&FmodPosition, &FmodVelocity);
        NewChannel->setVolume(Volume);
        NewChannel->setPitch(Pitch);
        NewChannel->setPaused(false);
        ActiveSFXChannels.Add(NewChannel);
    }
    return true;
}

bool USoundManager::PlayLoopingSFX(const FName& SoundName, const FString& FilePath, float Volume)
{
    if (!AudioSystem) return false;

    if (LoopingSFXChannels.Contains(SoundName))
    {
        FMOD::Channel** FoundChannelPtr = LoopingSFXChannels.Find(SoundName);
        FMOD::Channel* Channel = FoundChannelPtr ? *FoundChannelPtr : nullptr;
        if (Channel)
        {
            bool Paused = false; Channel->getPaused(&Paused);
            if (Paused) Channel->setPaused(false);
            Channel->setVolume(Volume);
            return true;
        }
    }

    // Create 2D looping sample
    FMOD::Sound* LoopSound = nullptr;
    FMOD_MODE Mode = FMOD_DEFAULT | FMOD_CREATESAMPLE | FMOD_LOOP_NORMAL | FMOD_2D;
    FMOD_RESULT Result = AudioSystem->createSound(FilePath.c_str(), Mode, nullptr, &LoopSound);
    if (!CheckFMODResult(Result, "CreateSound(LoopingSFX)")) return false;

    FMOD::Channel* NewChannel = nullptr;
    Result = AudioSystem->playSound(LoopSound, SFXChannelGroup, false, &NewChannel);
    if (!CheckFMODResult(Result, "playSound(LoopingSFX)"))
    {
        if (LoopSound) LoopSound->release();
        return false;
    }
    if (NewChannel)
    {
        NewChannel->setVolume(Volume);
        LoopingSFXSounds.Add(SoundName, LoopSound);
        LoopingSFXChannels.Add(SoundName, NewChannel);
        return true;
    }
    if (LoopSound) LoopSound->release();
    return false;
}

void USoundManager::StopLoopingSFX(const FName& SoundName)
{
    // AudioSystem이 내려간 상태에서도 내부 맵 정리는 수행하되, FMOD 호출은 건너뜁니다.
    if (LoopingSFXChannels.Contains(SoundName))
    {
        FMOD::Channel** FoundChannelPtr = LoopingSFXChannels.Find(SoundName);
        FMOD::Channel* Channel = FoundChannelPtr ? *FoundChannelPtr : nullptr;
        if (Channel && AudioSystem)
        {
            Channel->stop();
        }
        if (FoundChannelPtr) { *FoundChannelPtr = nullptr; }
        LoopingSFXChannels.Remove(SoundName);
    }
    if (LoopingSFXSounds.Contains(SoundName))
    {
        FMOD::Sound** FoundSoundPtr = LoopingSFXSounds.Find(SoundName);
        FMOD::Sound* Snd = FoundSoundPtr ? *FoundSoundPtr : nullptr;
        if (Snd && AudioSystem)
        {
            Snd->release();
        }
        if (FoundSoundPtr) { *FoundSoundPtr = nullptr; }
        LoopingSFXSounds.Remove(SoundName);
    }
}

void USoundManager::StopAllSounds()
{
    // Stop BGM
    if (CurrentBGMChannel)
    {
        CurrentBGMChannel->stop();
        CurrentBGMChannel = nullptr;
    }
    if (CurrentBGMSound)
    {
        CurrentBGMSound->release();
        CurrentBGMSound = nullptr;
    }
    CurrentBGMPath = FString();

    // Stop active one-shot SFX channels
    for (auto* Channel : ActiveSFXChannels)
    {
        if (Channel) { Channel->stop(); }
    }
    ActiveSFXChannels.Empty();

    // Stop and release looping SFX
    for (auto& Pair : LoopingSFXChannels)
    {
        if (Pair.second) { Pair.second->stop(); }
    }
    LoopingSFXChannels.Empty();
    for (auto& Pair : LoopingSFXSounds)
    {
        if (Pair.second) { Pair.second->release(); }
    }
    LoopingSFXSounds.Empty();
}
