#include <PathingVisualizer.h>

#include <PluginUtils.h>
#include <Rendering.h>

#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>

#include <GWCA/GameEntities/Agent.h>

#include <GWCA/Context/AgentContext.h>

#include <GWCA/GWCA.h>

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static PathingVisualizer instance;
    return &instance;
}
namespace 
{
    bool shouldRender() 
    {
        return GW::Map::GetInstanceType() == GW::Constants::InstanceType::Explorable && GW::Agents::GetControlledCharacter() && !GW::Agents::IsObserving();
    }
}

void PathingVisualizer::Initialize(ImGuiContext* ctx, ImGuiAllocFns fns, HMODULE toolbox_dll) {

    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
}

void PathingVisualizer::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();
}
bool PathingVisualizer::CanTerminate()
{
    return true;
}

void PathingVisualizer::Terminate()
{
    RenderingUtils::clearDrawingList();
    ToolboxUIPlugin::Terminate();
}

void PathingVisualizer::Draw(IDirect3DDevice9* device) 
{
    if (shouldRender())
        RenderingUtils::draw(device);
    else
        RenderingUtils::clearSingletonPolyline();
}

void PathingVisualizer::Update(float) 
{
    const auto agentMovement = [&]() -> const GW::AgentMovement* {
        const auto target = GW::Agents::GetTargetAsAgentLiving();
        const auto agentContext = GW::GetAgentContext();

        if (!shouldRender() || !target || !agentContext || agentContext->agent_movement.size() <= target->agent_id) return nullptr;
        return agentContext->agent_movement[target->agent_id];
    }();

    if (agentMovement) 
    {
        const auto makePos = [posPtr = reinterpret_cast<const float*>(agentMovement)](int offset) 
        {
            return GW::GamePos{*(posPtr + offset), *(posPtr + offset + 1), 0};
        };
        const auto color = ImGui::ColorConvertFloat4ToU32({1.f, 1.f, 1.f, 1.f});

        RenderingUtils::addSingletonPolyline({GW::Agents::GetTargetAsAgentLiving()->pos, makePos(34)}, color);
        // RenderingUtils::addSingletonPolyline({makePos(26), makePos(30), makePos(34), makePos(39)}, color);
    }
    else 
    {
        RenderingUtils::clearSingletonPolyline();
    }
}

void PathingVisualizer::DrawSettings() 
{
    ImGui::Text("Version 1.0. For new releases, feature requests and bug reports check out");
    ImGui::SameLine();
    constexpr auto discordInviteLink = "https://discord.gg/ZpKzer4dK9";
    ImGui::TextColored(ImColor{102, 187, 238, 255}, discordInviteLink);
    if (ImGui::IsItemClicked()) {
        ShellExecute(nullptr, "open", discordInviteLink, nullptr, nullptr, SW_SHOWNORMAL);
    }
}
void PathingVisualizer::LoadSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
}

void PathingVisualizer::SaveSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::SaveSettings(folder);

    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}
