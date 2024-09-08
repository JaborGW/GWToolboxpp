#include "PitsSoulsWindow.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>

#include <GWCA/Managers/AgentMgr.h>

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static PitsSoulsWindow instance;
    return &instance;
}
namespace {
    class PitsSoul {
    public:
        enum class State { Alive, Dead, Unknown };
        PitsSoul(GW::GamePos pos, std::string name) : pos{pos}, name{name} {}
        std::string print() const {
            switch (currentState) {
                case State::Alive:
                    return name + ": Death in " + std::to_string(static_cast<int>(103.6 * calculatedHpPercent)) + "s";
                case State::Dead:
                    return name + ": Respawn in " + std::to_string(static_cast<int>(secondsUntilRespawn)) + "s";
                default:
                    return name + ": Unknown";
            }
        }
        void update(GW::AgentLiving* a, std::chrono::time_point<std::chrono::steady_clock> now) {
            if (!a->GetIsAlive()) return;

            lastUpdateTime = now;
            readHpPercent = a->hp;
            currentState = State::Alive;
        }
        void update(std::chrono::time_point<std::chrono::steady_clock> now) {
            switch (currentState) {
                case State::Unknown:
                {
                    return;
                }
                case State::Alive: 
                {
                    const auto msSinceUpdate = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count();
                    calculatedHpPercent = readHpPercent - msSinceUpdate / 103'600;
                    if (calculatedHpPercent < 0) {
                        currentState = State::Dead;
                        deathTime = now;
                    }
                    return;
                }
                case State::Dead:
                {
                    const auto msSinceDeath = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now - deathTime).count();
                    secondsUntilRespawn = 120.0f - msSinceDeath / 1000;
                    if (secondsUntilRespawn < 0) {
                        currentState = State::Alive;
                        readHpPercent = 1;
                        lastUpdateTime = now;
                    }
                    return;
                }
            }
        }

        GW::GamePos pos;
        std::string name;
        State currentState = State::Unknown;
        double calculatedHpPercent = 0;
        double readHpPercent = 0;
        std::chrono::time_point<std::chrono::steady_clock> lastUpdateTime;
        std::chrono::time_point<std::chrono::steady_clock> deathTime;
        double secondsUntilRespawn = 0;
    };

    std::array<PitsSoul, 3> souls = {
        PitsSoul(GW::Vec2f{11427.f, 5079.f}, "Bottom"),
        PitsSoul(GW::Vec2f{ 9245.f, 4898.f}, "Double"), 
        PitsSoul(GW::Vec2f{10130.f, 6616.f}, "Reaper")
    };

    void SetNextWindowCenter(const ImGuiWindowFlags flags)
    {
        const auto& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), flags, ImVec2(0.5f, 0.5f));
    }
}

void PitsSoulsWindow::Update(float delay) 
{
    ToolboxUIPlugin::Update(delay);

    const auto agents = GW::Agents::GetAgentArray();
    if (!agents)
        return;

    const auto now = std::chrono::steady_clock::now();

    for (auto& soul : souls)
        soul.update(now);

    for (const auto& agent : *agents) {
        if (!agent) continue;
        const auto living = agent->GetAsAgentLiving();
        if (!living || living->player_number != GW::Constants::ModelID::UW::ChainedSoul || !living->GetIsAlive()) continue;

        for (auto& soul : souls) {
            if (GW::GetDistance(living->pos, soul.pos) < 50) {
                soul.update(living, now);
            }
        }
    }
}

void PitsSoulsWindow::Draw(IDirect3DDevice9* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
    if (!GetVisiblePtr() || !*GetVisiblePtr()) return;

    SetNextWindowCenter(ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(100, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(), GetVisiblePtr(), GetWinFlags())) {
        for (const auto& soul : souls)
            ImGui::Text("%s", soul.print().c_str());
    }
    ImGui::End();
}

void PitsSoulsWindow::DrawSettings()
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
