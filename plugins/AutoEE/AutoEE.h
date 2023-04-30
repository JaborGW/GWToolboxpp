#pragma once

#include <ToolboxPlugin.h>

class AutoEE : public ToolboxPlugin {
public:
    const char* Name() const override { return "AutoEE"; }

    void Update(float) override;
    
    void DrawSettings() override;
    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;

    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    bool CanTerminate() override;
    void SignalTerminate() override;
    void Terminate() override;

private:
    long shortcutKey = 0;
    long shortcutMod = 0;
    char hotkeyDescription[64];

    bool wasUsingShadowStep = false;
};
