#include "AutoEE.h"

#include <GWCA/GWCA.h>

#include <GWCA/Utilities/Hooker.h>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/GameEntities/Map.h>

#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Managers/AgentMgr.h>

#include <Keys.h>

#include <imgui.h>
#include <SimpleIni.h>
#include <filesystem>
#include <chrono>

namespace {
    using namespace std::chrono_literals;

    template<typename FuncT>
    void runWithDelay(FuncT&& function, std::chrono::milliseconds delay)
    {
        std::thread([function = std::move(function), delay] {
            std::this_thread::sleep_for(delay);
            function();
        }).detach();
    }
    bool usesShadowStep(const GW::AgentLiving* agent) { 
        using GW::Constants::SkillID;

        if (!agent)
            return false;

        const auto id = static_cast<GW::Constants::SkillID>(agent->skill);
        switch (id) {
            case SkillID::Deaths_Charge:
            case SkillID::Heart_of_Shadow:
            case SkillID::Vipers_Defense:
            case SkillID::Dark_Prison:
            case SkillID::Ebon_Escape:
            case SkillID::Aura_of_Displacement:
            case SkillID::Beguiling_Haze:
            case SkillID::Shadow_Fang:
            case SkillID::Shadow_Prison:
            case SkillID::Shadow_Theft:
            case SkillID::Wastrels_Collapse:
            case SkillID::Deaths_Retreat:
            case SkillID::Return:
            case SkillID::Shadow_Meld:
            case SkillID::Spirit_Walk:
            case SkillID::Swap:
                return true;
            default: 
                return false;
        }
    }
    bool jumpToAlly(GW::Constants::SkillID skillid, const GW::AgentLiving* target, std::chrono::milliseconds delay = 0ms) {
        const auto self = GW::Agents::GetPlayerAsAgentLiving();
        if (!self || self->energy * self->max_energy < 5.f)
            return false;
        if (self->agent_id == target->agent_id)
            return false;

        int islot = GW::SkillbarMgr::GetSkillSlot(skillid);
        if (islot < 0)
            return false;
        uint32_t slot = static_cast<uint32_t>(islot);

        GW::Skillbar* skillbar = GW::SkillbarMgr::GetPlayerSkillbar();
        if (!skillbar || !skillbar->IsValid())
            return false;
        if (skillbar->skills[slot].recharge != 0)
            return false;

        if (GW::GetSquareDistance(target->pos, self->pos) > GW::Constants::SqrRange::Spellcast)
            return false;

        auto castEE = [slot, id=target->agent_id] {
            GW::GameThread::Enqueue([slot, id]() -> void {
                GW::SkillbarMgr::UseSkill(slot, id);
            });
        };
        if (delay > 0ms)
            runWithDelay(std::move(castEE), delay);
        else
            castEE();
        return true;
    }
}

void AutoEE::DrawSettings()
{
    ToolboxPlugin::DrawSettings();

    const auto scale = ImGui::GetIO().FontGlobalScale;

    ImGui::Text("Hotkey:       ");
    ImGui::SameLine();
    if (ImGui::Button(hotkeyDescription)) {
        ImGui::OpenPopup("Select Hotkey");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Click to change hotkey");
    if (ImGui::BeginPopup("Select Hotkey")) {
        static long newkey = 0;
        char newkey_buf[256]{};
        long newmod = 0;

        ImGui::Text("Press key");
        if (ImGui::IsKeyDown(ImGuiKey_ModCtrl))
            newmod |= ModKey_Control;
        if (ImGui::IsKeyDown(ImGuiKey_ModShift))
            newmod |= ModKey_Shift;
        if (ImGui::IsKeyDown(ImGuiKey_ModAlt))
            newmod |= ModKey_Alt;

         if (newkey == 0) { // we are looking for the key
            for (WORD i = 0; i < 512; i++) {
                switch (i) {
                    case VK_CONTROL:
                    case VK_LCONTROL:
                    case VK_RCONTROL:
                    case VK_SHIFT:
                    case VK_LSHIFT:
                    case VK_RSHIFT:
                    case VK_MENU:
                    case VK_LMENU:
                    case VK_RMENU: continue;
                    default: {
                        if (::GetKeyState(i) & 0x8000)
                            newkey = i;
                    }
                }
            }
        }
        else if (!(::GetKeyState(newkey) & 0x8000)) { // We have a key, check if it was released
            shortcutKey = newkey;
            shortcutMod = newmod;
            ModKeyName(hotkeyDescription, _countof(hotkeyDescription), newmod, newkey);
            newkey = 0;
            ImGui::CloseCurrentPopup();
        }

        ModKeyName(newkey_buf, _countof(newkey_buf), newmod, newkey);
        ImGui::Text("%s", newkey_buf);

        ImGui::EndPopup();
    }
    ImGui::Text("Cast delay: ");
    ImGui::SameLine();
    if (ImGui::SliderInt("", &castDelayInMs, 0, 250)) {}
    ImGui::SameLine();
    ImGui::TextDisabled("%s", "(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s",
            "If you get a HCT, but your ally does not AND you have low ping, its possible that your EE ends before the shadow step. " 
            "Increase this value if you are having this issue. Decrease this value if the plugin stops working.");
    }
    
}

void AutoEE::LoadSettings(const wchar_t* folder)
{
    CSimpleIniA ini{};
    const auto path = std::filesystem::path(folder) / L"autoee.ini";
    ini.LoadFile(path.wstring().c_str());

    shortcutKey = ini.GetLongValue(Name(), "key", shortcutKey);
    shortcutMod = ini.GetLongValue(Name(), "mod", shortcutMod);
    castDelayInMs = ini.GetLongValue(Name(), "delay", castDelayInMs);

    ModKeyName(hotkeyDescription, _countof(hotkeyDescription), shortcutMod, shortcutKey);
}

void AutoEE::SaveSettings(const wchar_t* folder)
{
    CSimpleIniA ini{};
    const auto path = std::filesystem::path(folder) / L"autoee.ini";
    ini.SetLongValue(Name(), "key", shortcutKey);
    ini.SetLongValue(Name(), "mod", shortcutMod);
    ini.SetLongValue(Name(), "delay", castDelayInMs);

    ini.SaveFile(path.wstring().c_str());
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static AutoEE instance;
    return &instance;
}
void AutoEE::Update(float delta)
{
    ToolboxPlugin::Update(delta);

    if (GW::Map::GetInstanceType() != GW::Constants::InstanceType::Explorable)
        return;
    bool keyIsPressed = GetAsyncKeyState(shortcutKey) & (1 << 15);
    if (shortcutMod & ModKey_Control)
        keyIsPressed &= ImGui::IsKeyDown(ImGuiKey_ModCtrl);
    if (shortcutMod & ModKey_Shift)
        keyIsPressed &= ImGui::IsKeyDown(ImGuiKey_ModShift);
    if (shortcutMod & ModKey_Alt)
        keyIsPressed &= ImGui::IsKeyDown(ImGuiKey_ModAlt);

    if (keyIsPressed) {
        const auto target = GW::Agents::GetTargetAsAgentLiving();
        const auto isUsingShadowStep = usesShadowStep(target);
        if (isUsingShadowStep && !wasUsingShadowStep) { // Only cast on first frame
            bool hasJumped = jumpToAlly(GW::Constants::SkillID::Ebon_Escape, target, std::chrono::milliseconds{castDelayInMs});
            if (!hasJumped)
                hasJumped = jumpToAlly(GW::Constants::SkillID::Return, target, std::chrono::milliseconds{castDelayInMs});
            if (!hasJumped)
                hasJumped = jumpToAlly(GW::Constants::SkillID::Deaths_Retreat, target, std::chrono::milliseconds{castDelayInMs});
        }
        wasUsingShadowStep = isUsingShadowStep;
    }
    else
        wasUsingShadowStep = false;
}

void AutoEE::Initialize(ImGuiContext* ctx, ImGuiAllocFns fns, HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, fns, toolbox_dll);
    GW::Initialize();
}
void AutoEE::SignalTerminate() 
{ 
    GW::DisableHooks(); 
}
bool AutoEE::CanTerminate() 
{ 
    return GW::HookBase::GetInHookCount() == 0; 
}

void AutoEE::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::Terminate();
}
