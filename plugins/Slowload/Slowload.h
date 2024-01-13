#pragma once

#include <ToolboxPlugin.h>
#include <GWCA/Utilities/Hook.h>

class Slowload : public ToolboxPlugin {
public:
    const char* Name() const override { return "Slowload"; }

    void Update(float) override;
    
    void DrawSettings() override;
    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;

    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    bool CanTerminate() override;
    void SignalTerminate() override;
    void Terminate() override;

private:
    GW::HookEntry instanceLoadEntry;
};
