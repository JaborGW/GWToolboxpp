#include "PathingIndicator.h"

#include <GWCA/GameEntities/Pathing.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>

#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>

#include <GWCA/Utilities/Scanner.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Utilities/Hook.h>

#include <GWCA/Packets/StoC.h>

#include <Utils/FontLoader.h>

//DBG
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/ChatMgr.h>

namespace 
{
    GW::HookEntry skillCastEntry;
    GW::HookEntry instanceLoadEntry;

    typedef void(__cdecl* FindPath_pt)(PathPoint* start, PathPoint* goal, float range, uint32_t maxCount, uint32_t* count, PathPoint* pathArray);
    static FindPath_pt FindPath_Func = nullptr;
    static const GW::PathingMapArray* path_map = nullptr;

    uint32_t pathingCount = 9;
    float pathingRange = 10'000.f;
    constexpr auto sampleCount = 16;

    bool terminating = false;

    float Cross(const GW::Vec2f& lhs, const GW::Vec2f& rhs)
    {
        return (lhs.x * rhs.y) - (lhs.y * rhs.x);
    }

    enum class PathingResult { CanPath, CanPartiallyPath, CannotPath, Unknown };
    OutcomeChances toChances(PathingResult res) {
        OutcomeChances c;
        switch (res) 
        {
            case PathingResult::CanPath:        
                c.success = 1.f;
                break;
            case PathingResult::CanPartiallyPath:
                c.partial = 1.f;
                break;
            case PathingResult::CannotPath:
            case PathingResult::Unknown:
                c.failure = 1.f;
                break;
        }
        return c;
    }

    void logMessage(std::string_view message)
    {
        const auto wMessage = std::wstring{message.begin(), message.end()};
        const size_t len = 42 + wcslen(wMessage.c_str());
        auto to_send = new wchar_t[len];
        swprintf(to_send, len - 1, L"<a=1>%s</a><c=#%6X>: %s</c>", L"Slowload plugin", 0xFFFFFF, wMessage.c_str());
        GW::GameThread::Enqueue([to_send] {
            GW::Chat::WriteChat(GW::Chat::Channel::CHANNEL_GWCA2, to_send, nullptr);
            delete[] to_send;
        });
    }

    bool pointOnTrapezoid(const GW::GamePos& p, const GW::PathingTrapezoid& trap)
    {
        //  a----d
        //   \    \
        //    b____c
        const auto a = GW::Vec2f{trap.XTL, trap.YT};
        const auto b = GW::Vec2f{trap.XBL, trap.YB};
        const auto c = GW::Vec2f{trap.XBR, trap.YB};
        const auto d = GW::Vec2f{trap.XTR, trap.YT};

        // See GWCA pathing.cpp:IsOnPathingTrapezoid
        constexpr float tolerance = 2.0f;
        if (a.y < p.y || b.y > p.y) return false;
        if (b.x > p.x && a.x > p.x) return false;
        if (c.x < p.x && d.x < p.x) return false;
        const auto ab = b - a, cd = d - c, pa = a - p, pc = c - p;
        if (Cross(ab, pa) > tolerance) return false;
        if (Cross(cd, pc) > tolerance) return false;
        return true;
    }

    const GW::PathingTrapezoid* findTrapezoid(const GW::GamePos& pos)
    {
        for (const auto& map : *path_map) {
            for (uint32_t i = 0u; i < map.trapezoid_count; ++i) {
                if (pointOnTrapezoid(pos, map.trapezoids[i])) {
                    return &map.trapezoids[i];
                }
            }
        }
        return nullptr;
    }

    /// @param direction: int 0..15, picks direction to offset to
    GW::GamePos getOffsetPosition(GW::GamePos center, float distance, uint8_t direction)
    {
        constexpr auto pi = 3.14159265359f;
        const auto phi = direction * pi / sampleCount;
        center.y += distance * std::sin(phi);
        center.x += distance * std::cos(phi);
        return center;
    }
    PathingResult canPathToTarget(const PathPoint& playerPathPoint, const PathPoint& targetPos)
    {
        auto start = playerPathPoint;
        auto end = targetPos;

        std::vector<PathPoint> pathArray(pathingCount);
        uint32_t count = pathArray.size();
        FindPath_Func(&start, &end, pathingRange, count, &count, &pathArray[0]);
        logMessage("Target: " + std::to_string(targetPos.pos.x) + std::to_string(targetPos.pos.y));
        logMessage("Last: " + std::to_string(pathArray[count - 1].pos.x) + std::to_string(pathArray[count - 1].pos.y));
        return (GW::GetSquareDistance(pathArray[count - 1].pos, end.pos) < 200) ? PathingResult::CanPath : PathingResult::CannotPath;
    }
    PathingResult canPathToTarget(const PathPoint& playerPathPoint, const GW::GamePos& targetPos)
    {
        const auto trap = findTrapezoid(targetPos);
        if (!trap) return PathingResult::Unknown;
        return canPathToTarget(playerPathPoint, {targetPos, trap});
    }
    OutcomeChances dcPrediction(const PathPoint& playerPathPoint, const GW::Agent* target)
    {
        if (!target) return {};
        
        constexpr float dcOffset = 100.f;

        OutcomeChances chances;
        const auto handleResult = [&](PathingResult res) {
            switch (res) {
                case PathingResult::CanPath:
                    chances.success += 1. / sampleCount;
                    break;
                case PathingResult::CanPartiallyPath:
                    chances.partial += 1. / sampleCount;
                    break;
                case PathingResult::CannotPath:
                case PathingResult::Unknown:
                    chances.failure += 1. / sampleCount;
                    break;
            }
        };

        for (uint8_t direction = 0u; direction < sampleCount; ++direction) 
        {
            const auto targetPos = getOffsetPosition(target->pos, dcOffset, direction);
            handleResult(canPathToTarget(playerPathPoint, targetPos));
        }

        return chances;
    }

    GW::UI::Frame* skillbar_frame = nullptr;
    bool skillbar_position_dirty = true;
    GW::UI::UIInteractionCallback OnSkillbar_UICallback_Ret = nullptr;
    GW::UI::FramePosition skillbar_skill_positions[8];
    ImVec2 skill_positions_calculated[8];
    float m_skill_width = 50.f;
    float m_skill_height = 50.f;
    enum class Layout { Row, Rows, Column, Columns };
    Layout layout = Layout::Row;
    GW::HookEntry OnUIMessage_HookEntry;
    void __cdecl OnSkillbar_UICallback(GW::UI::InteractionMessage* message, void* wParam, void* lParam)
    {
        GW::Hook::EnterHook();
        OnSkillbar_UICallback_Ret(message, wParam, lParam);
        switch (static_cast<uint32_t>(message->message_id)) {
            case 0xb:
                skillbar_frame = nullptr;
                skillbar_position_dirty = true;
                break;
            case 0x13:
            case 0x30:
            case 0x33:
                skillbar_position_dirty = true; // Forces a recalculation
                break;
        }
        GW::Hook::LeaveHook();
    }
    GW::UI::Frame* GetSkillbarFrame()
    {
        if (skillbar_frame) return skillbar_frame;
        skillbar_frame = GW::UI::GetFrameByLabel(L"Skillbar");
        if (skillbar_frame) {
            logMessage("Frame_callbacks size" + std::to_string(skillbar_frame->frame_callbacks.size()));
            if (skillbar_frame->frame_callbacks[0] != OnSkillbar_UICallback) {
                OnSkillbar_UICallback_Ret = skillbar_frame->frame_callbacks[0];
                skillbar_frame->frame_callbacks[0] = OnSkillbar_UICallback;
            }
        }
        return skillbar_frame;
    }

    bool GetSkillbarPos()
    {
        if (!skillbar_position_dirty) return true;
        const auto frame = GetSkillbarFrame();
        if (!(frame && frame->IsVisible() && frame->IsCreated())) {
            return false;
        }
        if (!GImGui) return false;
        // Imgui viewport may not be limited to the game area.
        const auto imgui_viewport = ImGui::GetMainViewport();

        for (size_t i = 0; i < _countof(skillbar_skill_positions); i++) {
            const auto skillframe = GW::UI::GetChildFrame(frame, i);
            if (!skillframe) return false;
            skillbar_skill_positions[i] = skillframe->position;
            skill_positions_calculated[i] = skillbar_skill_positions[i].GetTopLeftOnScreen();
            skill_positions_calculated[i].y += imgui_viewport->Pos.y;
            skill_positions_calculated[i].x += imgui_viewport->Pos.x;
            if (i == 0) {
                m_skill_width = skillbar_skill_positions[0].GetSizeOnScreen().x;
                m_skill_height = skillbar_skill_positions[0].GetSizeOnScreen().y;
            }
        }

        // Calculate columns/rows
        if (skillbar_skill_positions[0].screen_top == skillbar_skill_positions[7].screen_top) {
            layout = Layout::Row;
        }
        else if (skillbar_skill_positions[0].screen_left == skillbar_skill_positions[7].screen_left) {
            layout = Layout::Column;
        }
        else if (skillbar_skill_positions[0].screen_top == skillbar_skill_positions[3].screen_top) {
            layout = Layout::Rows;
        }
        else {
            layout = Layout::Columns;
        }
        skillbar_position_dirty = false;
        return true;
    }
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static PathingIndicator instance;
    return &instance;
}

void PathingIndicator::Update(float delta)
{
    ToolboxUIPlugin::Update(delta);

    const auto player = GW::Agents::GetControlledCharacter();

    if (!FindPath_Func || !player || GW::Map::GetInstanceType() != GW::Constants::InstanceType::Explorable)
    {
        chances.clear();
        return;
    }
    if (!path_map) 
    {
        path_map = GW::Map::GetPathingMap();
    }
    if (!path_map) 
    {
        return;
    }
    const auto playerPathPoint = PathPoint{player->pos, findTrapezoid(player->pos)};

    if (GW::Effects::GetPlayerEffectBySkillId(GW::Constants::SkillID::Shadow_of_Haste)) 
    {
        chances[GW::Constants::SkillID::Shadow_of_Haste] = toChances(canPathToTarget(playerPathPoint, shadowOfHasteLocation));
    }
    else 
    {
        chances.erase(GW::Constants::SkillID::Shadow_of_Haste);
    }

    /* if (GW::Effects::GetPlayerEffectBySkillId(GW::Constants::SkillID::Shadow_Walk)) 
    {
        chances[GW::Constants::SkillID::Shadow_Walk] = toChances(canPathToTarget(playerPathPoint, shadowOfHasteLocation));
    }
    else 
    {
        chances.erase(GW::Constants::SkillID::Shadow_Walk);
    }

    if (const auto recall = GW::Effects::GetPlayerEffectBySkillId(GW::Constants::SkillID::Recall)) 
    {
        if (const auto recallTarget = GW::Agents::GetAgentByID(recall->agent_id)) 
        {
            chances[GW::Constants::SkillID::Recall] = dcPrediction(playerPathPoint, recallTarget);
        }
        else 
        {
            chances.erase(GW::Constants::SkillID::Recall);
        }
    }
    else 
    {
        chances.erase(GW::Constants::SkillID::Recall);
    }

    const auto dcSlot = GW::SkillbarMgr::GetSkillSlot(GW::Constants::SkillID::Deaths_Charge);
    if (dcSlot != -1) 
    {
        if (const auto target = GW::Agents::GetTarget()) 
        {
            chances[GW::Constants::SkillID::Deaths_Charge] = dcPrediction(playerPathPoint, target);
        }
        else 
        {
            chances.erase(GW::Constants::SkillID::Deaths_Charge);
        }
    }
    else 
    {
        chances.erase(GW::Constants::SkillID::Deaths_Charge);
    }*/
}

void PathingIndicator::Draw(IDirect3DDevice9*) 
{
    if (terminating || !GetVisiblePtr() || !*GetVisiblePtr())
        return;
    if (skillbar_position_dirty && !GetSkillbarPos()) {
        return; // Failed to get skillbar pos
    }
    const auto skillbar = GW::SkillbarMgr::GetPlayerSkillbar();
    if (!skillbar) return;

    //const auto font = FontLoader::GetFont(FontLoader::FontSize::header1);
    const auto draw_list = ImGui::GetBackgroundDrawList();
    //draw_list->PushTextureID(font->ContainerAtlas->TexID);

    for (size_t i = 0; i < 8; i++) {
        if (!chances.contains(skillbar->skills[i].skill_id))
            continue;
        // NB: Y axis inverted for imgui
        const ImVec2& top_left = skill_positions_calculated[i];
        const ImVec2 bottom_right = {skill_positions_calculated[i].x + m_skill_width, skill_positions_calculated[i].y + m_skill_height};

        // draw overlay
        const auto color = chances[skillbar->skills[i].skill_id].success > 0.9f ? ImGui::ColorConvertFloat4ToU32({0.f, 1.f, 0.f, 0.4f}) : ImGui::ColorConvertFloat4ToU32({1.f, 0.f, 0.f, 0.4f});
        draw_list->AddRectFilled(top_left, bottom_right, color);
    }
    //draw_list->PopTextureID();
}

void PathingIndicator::LoadSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
}

void PathingIndicator::SaveSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    
}

void PathingIndicator::DrawSettings()
{
    if (!toolbox_handle) {
        return;
    }
    ToolboxUIPlugin::DrawSettings();

    ImGui::InputFloat("Pathing range", &pathingRange);
    ImGui::InputInt("Pathing step count", reinterpret_cast<int*>(&pathingCount));
}

void PathingIndicator::Initialize(ImGuiContext* ctx, const ImGuiAllocFns allocator_fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, allocator_fns, toolbox_dll);
    
    FindPath_Func = (FindPath_pt)GW::Scanner::Find("\x83\xec\x20\x53\x56\x57\xe8\x92\x8a\xdd", "xxxxxxxxxx", -0x3);
    
    GW::UI::RegisterUIMessageCallback(&skillCastEntry, GW::UI::UIMessage::kSkillActivated, [this](GW::HookStatus*, const GW::UI::UIMessage, void* wParam, void*) 
    {
        struct Payload {
            uint32_t agent_id;
            GW::Constants::SkillID skill_id;
        };
        const auto payload = *static_cast<Payload*>(wParam);
        const auto player = GW::Agents::GetControlledCharacter();
        if (!player || payload.agent_id != player->agent_id) return;

        switch (payload.skill_id) 
        {
            case GW::Constants::SkillID::Shadow_of_Haste:
                shadowOfHasteLocation = {player->pos, findTrapezoid(player->pos)};
                break;
            case GW::Constants::SkillID::Shadow_Walk:
                shadowWalkLocation = {player->pos, findTrapezoid(player->pos)};
                break;
            default:
                return;
        }
    });
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::InstanceLoadFile>(&instanceLoadEntry, [this](GW::HookStatus*, const GW::Packet::StoC::InstanceLoadFile*) {
        chances.clear();
        shadowOfHasteLocation = {};
        shadowWalkLocation = {};
        path_map = nullptr;
    });
    GW::UI::RegisterUIMessageCallback(&OnUIMessage_HookEntry, GW::UI::UIMessage::kUIPositionChanged, [&](GW::HookStatus*, GW::UI::UIMessage, void*, void*) { skillbar_position_dirty = true; }, 0x8000);
    GW::UI::RegisterUIMessageCallback(&OnUIMessage_HookEntry, GW::UI::UIMessage::kPreferenceValueChanged, [&](GW::HookStatus*, GW::UI::UIMessage, void*, void*) { skillbar_position_dirty = true; }, 0x8000);
}

void PathingIndicator::SignalTerminate()
{
    terminating = true;
    ToolboxUIPlugin::SignalTerminate();
    
    GW::UI::RemoveUIMessageCallback(&OnUIMessage_HookEntry, GW::UI::UIMessage::kPreferenceValueChanged);
    GW::UI::RemoveUIMessageCallback(&OnUIMessage_HookEntry, GW::UI::UIMessage::kUIPositionChanged);
    GW::StoC::RemoveCallbacks(&instanceLoadEntry);
    GW::UI::RemoveUIMessageCallback(&skillCastEntry, GW::UI::UIMessage::kSkillActivated);
}

void PathingIndicator::Terminate() 
{
    if (skillbar_frame && skillbar_frame->frame_callbacks[0] == OnSkillbar_UICallback) 
    {
        skillbar_frame->frame_callbacks[0] = OnSkillbar_UICallback_Ret;
        skillbar_frame = nullptr;
    }
    ToolboxUIPlugin::Terminate();
}

bool PathingIndicator::CanTerminate() 
{
    return GW::Hook::GetInHookCount() == 0;
}
