#include "PathingIndicator.h"

#include <GWCA/GameEntities/Pathing.h>
#include <GWCA/GameEntities/Agent.h>

#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/AgentMgr.h>

#include <GWCA/Utilities/Scanner.h>
#include <GWCA/Utilities/Hooker.h>

namespace 
{
    struct PathPoint {
        GW::GamePos pos = {};
        const GW::PathingTrapezoid* t = nullptr;
    };
    typedef void(__cdecl* FindPath_pt)(PathPoint* start, PathPoint* goal, float range, uint32_t maxCount, uint32_t* count, PathPoint* pathArray);
    static FindPath_pt FindPath_Func = nullptr;
    constexpr uint32_t pathingCount = 9;
    constexpr float pathingRange = 10'000.f;

    float Cross(const GW::Vec2f& lhs, const GW::Vec2f& rhs)
    {
        return (lhs.x * rhs.y) - (lhs.y * rhs.x);
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

    const GW::PathingTrapezoid* findTrapezoid(const GW::GamePos& pos, const GW::PathingMapArray* path_map)
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

    enum class PathingResult { CanPath, CannotPath, Unknown };
    /// @param direction: int 0..15, picks direction to offset to
    GW::GamePos getOffsetPosition(GW::GamePos center, float distance, uint8_t direction)
    {
        constexpr auto pi = 3.14159265359f;
        const auto phi = direction * pi / 16;
        center.y += distance * std::sin(phi);
        center.x += distance * std::cos(phi);
        return center;
    }
    PathingResult canPathToTarget(const PathPoint& playerPathPoint, const GW::GamePos& targetPos, const GW::PathingMapArray* path_map)
    {
        auto start = playerPathPoint;
        auto end = PathPoint{targetPos, findTrapezoid(targetPos, path_map)};
        if (!end.t) return PathingResult::Unknown;

        std::array<PathPoint, pathingCount> pathArray;
        uint32_t count = pathArray.size();
        FindPath_Func(&start, &end, pathingRange, count, &count, &pathArray[0]);
        return (GW::GetSquareDistance(pathArray[count - 1].pos, end.pos) < 200 || count > 1) ? PathingResult::CanPath : PathingResult::CannotPath;
    }
    PathingResult sohResult(const GW::GamePos& sohPoint, const GW::PathingMapArray* path_map) 
    {
        const auto player = GW::Agents::GetControlledCharacter();
        if (!player || !path_map) return PathingResult::Unknown;
        const auto playerPathPoint = PathPoint{player->pos, findTrapezoid(player->pos, path_map)};
        return canPathToTarget(playerPathPoint, sohPoint, path_map);
    }
    std::optional<float> dcSuccessRate(const GW::AgentLiving* target, const GW::PathingMapArray* path_map)
    {
        const auto player = GW::Agents::GetControlledCharacter();
        if (!target || !player) return;

        int sucessful = 0;
        int results = 0;
        auto handleResult = [&](PathingResult res) {
            switch (res) {
                case PathingResult::CanPath:
                    ++sucessful;
                    ++results;
                    break;
                case PathingResult::CannotPath:
                    ++results;
                    break;
                case PathingResult::Unknown:
                    break;
            }
        };
        constexpr float dcOffset = 100.f;

        const auto playerPathPoint = PathPoint{player->pos, findTrapezoid(player->pos, path_map)};
        for (uint8_t direction = 0u; direction < 16; ++direction) 
        {
            const auto targetPos = getOffsetPosition(target->pos, dcOffset, direction);
            handleResult(canPathToTarget(playerPathPoint, targetPos, path_map));
        }

        if (results == 0) return std::nullopt;
        return (float)sucessful / results;
    }
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static PathingIndicator instance;
    return &instance;
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
}

void PathingIndicator::Initialize(ImGuiContext* ctx, const ImGuiAllocFns allocator_fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, allocator_fns, toolbox_dll);

    FindPath_Func = (FindPath_pt)GW::Scanner::Find("\x83\xec\x20\x53\x8b\x5d\x1c\x56\x57\xe8", "xxxxxxxxxx", -0x3);
}
void PathingIndicator::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    
}

bool PathingIndicator::CanTerminate() 
{
    return GW::Hook::GetInHookCount() == 0;
}

void PathingIndicator::Draw(IDirect3DDevice9*)
{
    
}
