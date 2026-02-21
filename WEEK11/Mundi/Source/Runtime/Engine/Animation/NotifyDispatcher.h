#pragma once
#include "Name.h"
#include "UEContainer.h"
#include <functional>

struct FAnimNotifyEvent;

// Global dispatcher for animation notifies. Stores handlers by sequence key (file path) and notify name.
class FNotifyDispatcher
{
public:
    using FNotifyHandler = std::function<void(const FAnimNotifyEvent&)>;

    static FNotifyDispatcher& Get();

    void Enable(bool bInEnabled) { bEnabled = bInEnabled; }
    bool IsEnabled() const { return bEnabled; }

    void Clear();
    void Register(const FString& SeqKey, const FName& NotifyName, const FNotifyHandler& Handler);
    void Dispatch(const FString& SeqKey, const FAnimNotifyEvent& Event);

private:
    TMap<FString, TMap<FName, FNotifyHandler>> HandlersBySeq;
    bool bEnabled = true;
};


