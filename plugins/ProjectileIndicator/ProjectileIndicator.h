#pragma once

#include <ToolboxUIPlugin.h>

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/Constants/Constants.h>

class ProjectileIndicator : public ToolboxUIPlugin {
public:
    ProjectileIndicator() = default;
    ~ProjectileIndicator() override = default;

    const char* Name() const override { return "ProjectileIndicator"; }

    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;

    void DrawSettings() override;
    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;

    void Draw(IDirect3DDevice9*) override;

private:
    bool filled = false;
    ImVec4 color = {1.f, 0.f, 0.f, 1.f};
    int projectileTimer = 1000;
    std::vector<GW::Constants::SkillID> trackedSkills = {GW::Constants::SkillID::Bone_Spike, GW::Constants::SkillID::Flurry_of_Splinters};
    std::vector<int> trackedEnemyModels = {};
};
