#pragma once

#include <GWCA/Utilities/Hook.h>
#include <ToolboxUIPlugin.h>

class Slowload : public ToolboxUIPlugin {
public:
    Slowload()
    {
        show_closebutton = false;
        show_title = false;
        can_collapse = false;
        can_close = false;
        is_resizable = true;
        is_movable = true;
        lock_size = false;
        lock_move = false;
    }
    const char* Name() const override { return "Slowload"; }

    void Update(float) override;

    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    bool CanTerminate() override;
    void SignalTerminate() override;
    void Terminate() override;

    void DrawSettings() override;
    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;

    void Draw(IDirect3DDevice9*) override;

private:
    long shortcutKey = 1231231245;
    long shortcutMod = 1254123123;

    bool is_active = false;
    bool keyWasPressed = false;

    GW::HookEntry instanceLoadEntry;

    std::wstring slowloading_character_name = L"";
    char hotkeyDescription[64]{};
};
