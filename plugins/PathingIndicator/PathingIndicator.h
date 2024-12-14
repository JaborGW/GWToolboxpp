#pragma once

#include <ToolboxUIPlugin.h>

#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Pathing.h>

#include <unordered_map>

struct OutcomeChances {
    float success = 0.f;
    float partial = 0.f;
    float failure = 0.f;
};

struct PathPoint {
    GW::GamePos pos = {};
    const GW::PathingTrapezoid* t = nullptr;
};

class PathingIndicator : public ToolboxUIPlugin {
public:
    PathingIndicator()
    {
        can_show_in_main_window = false;
        show_title = false;
        can_collapse = false;
        can_close = false;
    }
    ~PathingIndicator() override = default;

    const char* Name() const override { return "Pathing Indicator"; }

    void Draw(IDirect3DDevice9* pDevice) override;
    void Update(float) override;

    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;
    void DrawSettings() override;

    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;

private:
    std::unordered_map<GW::Constants::SkillID, OutcomeChances> chances;
    PathPoint shadowOfHasteLocation{};
    PathPoint shadowWalkLocation{};
    bool skillbar_position_dirty = true;
};
