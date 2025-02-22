#include <ProjectileIndicator.h>

#include <PluginUtils.h>
#include <Rendering.h>

#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Managers/UIMgr.h>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>

#include <GWCA/GWCA.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hooker.h>

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static ProjectileIndicator instance;
    return &instance;
}
namespace 
{
    GW::HookEntry projectileHook;
    GW::HookEntry instanceLoadFileHook;

    constexpr auto ellipsisA = 125.f;
    constexpr auto ellipsisB = 50.f;
    
    std::string getSkillName(GW::Constants::SkillID id)
    {
        if (id == GW::Constants::SkillID::No_Skill) return "Auto-Attack";
        static std::unordered_map<GW::Constants::SkillID, std::wstring> decodedNames;
        if (const auto it = decodedNames.find(id); it != decodedNames.end()) {
            return PluginUtils::WStringToString(it->second);
        }

        const auto skillData = GW::SkillbarMgr::GetSkillConstantData(id);
        if (!skillData || (uint32_t)id >= (uint32_t)GW::Constants::SkillID::Count) return "";

        wchar_t out[8] = {0};
        if (GW::UI::UInt32ToEncStr(skillData->name, out, _countof(out))) {
            GW::UI::AsyncDecodeStr(out, &decodedNames[id]);
        }
        return "";
    }
}

void ProjectileIndicator::Initialize(ImGuiContext* ctx, ImGuiAllocFns fns, HMODULE toolbox_dll) {

    ToolboxPlugin::Initialize(ctx, fns, toolbox_dll);
    GW::Initialize();

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::AgentProjectileLaunched>(&projectileHook, [this](GW::HookStatus*, GW::Packet::StoC::AgentProjectileLaunched* packet) -> void {
        const auto agent = GW::Agents::GetAgentByID(packet->agent_id);
        if (!agent || !agent->GetIsLivingType()) return;
        const auto living = agent->GetAsAgentLiving();
        if (living->allegiance != GW::Constants::Allegiance::Enemy || !std::ranges::contains(trackedSkills, (GW::Constants::SkillID)agent->GetAsAgentLiving()->skill)) return;
        
        const auto ADirection = GW::Normalize(GW::Vec2f{GW::Agents::GetControlledCharacter()->pos} - GW::Vec2f{living->pos});
        RenderingUtils::addEllipseToDraw(packet->destination, ADirection, ellipsisA, ellipsisB, ImGui::ColorConvertFloat4ToU32(color), filled, projectileTimer);        
    });

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::InstanceLoadFile>(&instanceLoadFileHook, [](auto*, auto*) -> void 
    {
        RenderingUtils::clearDrawingList();
    });
    
}
void ProjectileIndicator::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();

    GW::StoC::RemoveCallback<GW::Packet::StoC::AgentProjectileLaunched>(&projectileHook);
    GW::StoC::RemoveCallback<GW::Packet::StoC::InstanceLoadStart>(&instanceLoadFileHook);
    GW::DisableHooks();
}
bool ProjectileIndicator::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void ProjectileIndicator::Terminate()
{
    RenderingUtils::clearDrawingList();
    ToolboxPlugin::Terminate();
    GW::Terminate();
}

void ProjectileIndicator::Draw(IDirect3DDevice9* device) 
{
    RenderingUtils::draw(device);
}

void ProjectileIndicator::DrawSettings() 
{
    ImGui::Checkbox("Fill circle", &filled);
    ImGui::ColorEdit4("Color", reinterpret_cast<float*>(&color));
    ImGui::SliderInt("Display duration (ms)", &projectileTimer, 100, 4000);

    ImGui::Text("Tracked skills:");
    auto toErase = trackedSkills.end();
    for (auto it = trackedSkills.begin(); it != trackedSkills.end(); ++it) {
        ImGui::PushID(it - trackedSkills.begin());
        ImGui::Bullet();
        if (ImGui::Button("X", ImVec2(20, 20))) 
        {
            toErase = it;
        }
        ImGui::SameLine();
        ImGui::Text(getSkillName(*it).c_str());
        ImGui::SameLine();
        ImGui::PushItemWidth(100.f);
        ImGui::InputInt("", reinterpret_cast<int*>(&*it), 0);
        ImGui::PopItemWidth();
        ImGui::PopID();
    }
    if (toErase != trackedSkills.end()) 
    {
        trackedSkills.erase(toErase);
    }
    if (ImGui::Button("+"))
    {
        trackedSkills.push_back(GW::Constants::SkillID::No_Skill);
    }

    ImGui::Text("Version 1.0. For new releases, feature requests and bug reports check out");
    ImGui::SameLine();
    constexpr auto discordInviteLink = "https://discord.gg/ZpKzer4dK9";
    ImGui::TextColored(ImColor{102, 187, 238, 255}, discordInviteLink);
    if (ImGui::IsItemClicked()) {
        ShellExecute(nullptr, "open", discordInviteLink, nullptr, nullptr, SW_SHOWNORMAL);
    }
}
void ProjectileIndicator::LoadSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::LoadSettings(folder);

    const auto loadColor = [&](ImVec4& color, std::string varName) {
        color.x = (float)ini.GetDoubleValue(Name(), (varName + "x").c_str(), color.x);
        color.y = (float)ini.GetDoubleValue(Name(), (varName + "y").c_str(), color.y);
        color.z = (float)ini.GetDoubleValue(Name(), (varName + "z").c_str(), color.z);
        color.w = (float)ini.GetDoubleValue(Name(), (varName + "w").c_str(), color.w);
    };
    loadColor(color, VAR_NAME(color));

    filled = ini.GetBoolValue(Name(), VAR_NAME(filled), filled);
    projectileTimer = ini.GetLongValue(Name(), VAR_NAME(projectileTimer), projectileTimer);

    const auto importedSkills = std::string{ini.GetValue(Name(), "skills", "")};
    std::istringstream iss(importedSkills);
    std::string item;
    trackedSkills.clear();
    while (std::getline(iss, item, ' '))
    {
        trackedSkills.push_back((GW::Constants::SkillID)std::stoi(item));
    }
    
}

void ProjectileIndicator::SaveSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::SaveSettings(folder);

    const auto saveColor = [&](const ImVec4& color, std::string varName) {
        ini.SetDoubleValue(Name(), (varName + "x").c_str(), color.x);
        ini.SetDoubleValue(Name(), (varName + "y").c_str(), color.y);
        ini.SetDoubleValue(Name(), (varName + "z").c_str(), color.z);
        ini.SetDoubleValue(Name(), (varName + "w").c_str(), color.w);
    };
    saveColor(color, VAR_NAME(color));
    ini.SetBoolValue(Name(), VAR_NAME(filled), filled);
    ini.SetLongValue(Name(), VAR_NAME(projectileTimer), projectileTimer);

    std::string skills;
    for (const auto& skill : trackedSkills) 
    {
        skills += std::to_string((int)skill) + " ";
    }
    ini.SetValue(Name(), "skills", skills.c_str());

    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}
