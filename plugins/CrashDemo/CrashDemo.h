#pragma once

#include <ToolboxPlugin.h>

class CrashDemo : public ToolboxPlugin {
public:
    const char* Name() const override { return "CrashDemo"; }

    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;
};
