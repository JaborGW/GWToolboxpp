#include "FlatBowRangeIndicator.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Utilities/Hook.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/GWCA.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/StoC.h>

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static FlatBowRangeIndicator instance;
    return &instance;
}
namespace {
    void SetNextWindowCenter(const ImGuiWindowFlags flags)
    {
        const auto& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), flags, ImVec2(0.5f, 0.5f));
    }

    float getAltitude(const GW::AgentLiving* agent)
    {
        if (!agent) return 0.f;

        float altitude = 0;
        GW::Map::QueryAltitude(agent->pos, 10, altitude);
        return -altitude;
    };

    enum class Result {InRange, OutOfRange, NoValidTarget};
    Result targetInBowRange() {
        const auto player = GW::Agents::GetControlledCharacter();
        const auto currentTarget = GW::Agents::GetTargetAsAgentLiving();
        if (player && currentTarget && currentTarget->allegiance == GW::Constants::Allegiance::Enemy) {
            const auto altitudeDifference = getAltitude(player) - getAltitude(currentTarget);
            const auto maxAttackRange = std::min(2000., altitudeDifference * (2e-4 * altitudeDifference + 1) + 1498.);

            const auto distance = GW::GetDistance(player->pos, currentTarget->pos);
            return distance <= maxAttackRange ? Result::InRange : Result::OutOfRange;
        }
        return Result::NoValidTarget;
    }
} // namespace

namespace ImGui {
    void TextShadowed(const char* label, ImVec2 offset = {1, 1}, const ImVec4& shadow_color = {0, 0, 0, 1})
    {
        const ImVec2 pos = GetCursorPos();
        ImGui::PushStyleColor(ImGuiCol_Text, shadow_color);
        SetCursorPos(ImVec2(pos.x + offset.x, pos.y + offset.y));
        TextUnformatted(label);
        ImGui::PopStyleColor();
        SetCursorPos(pos);
        TextUnformatted(label);
    }

}

void FlatBowRangeIndicator::Draw(IDirect3DDevice9* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
    if (!GetVisiblePtr() || !*GetVisiblePtr()) return;

    SetNextWindowCenter(ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);

    const auto result = targetInBowRange();
    if (result != Result::NoValidTarget) {
         if (ImGui::Begin(Name(), nullptr, GetWinFlags())){
            auto drawList = ImGui::GetForegroundDrawList();
            const auto pos = ImGui::GetWindowPos();
            const auto size = ImGui::GetWindowSize();
            constexpr auto green = ImVec4{106.f/255.f, 168.f/255.f,  79.f/255.f, 1.f};
            constexpr auto red   = ImVec4{224.f/255.f, 102.f/255.f, 102.f/255.f, 1.f};
            drawList->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y}, ImGui::ColorConvertFloat4ToU32(result == Result::InRange ? green : red), ImDrawListFlags_AntiAliasedFill);
        }
        ImGui::End();
    }
}

void FlatBowRangeIndicator::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings();

    ImGui::Text("Version 1.0. For new releases, feature requests and bug reports check out");
    ImGui::SameLine();

    constexpr auto discordInviteLink = "https://discord.gg/ZpKzer4dK9";
    ImGui::TextColored(ImColor{102, 187, 238, 255}, discordInviteLink);
    if (ImGui::IsItemClicked()) {
        ShellExecute(nullptr, "open", discordInviteLink, nullptr, nullptr, SW_SHOWNORMAL);
    }
}
