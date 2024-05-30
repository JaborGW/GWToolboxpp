#pragma once

#include <cstdint>

enum class Trigger { None, InstanceLoad, HardModePing, Hotkey };
enum class Class { Any, Warrior, Ranger, Monk, Necro, Mesmer, Elementalist, Assassin, Ritualist, Paragon, Dervish };
enum class Status { Any, Dead, Alive };
enum class AgentType { Any, Self, PartyMember, Friendly, Hostile };
enum class Sorting { AgentId, ClosestToPlayer, FurthestFromPlayer, ClosestToTarget, FurthestFromTarget, LowestHp, HighestHp };
enum class HexedStatus { Any, NotHexed, Hexed };
enum class Channel { All, Guild, Team, Trade, Alliance, Whisper, Emote };
enum class QuestStatus { NotStarted, Started, Completed, Failed };
enum class GoToTargetFinishCondition { None, StoppedMovingNextToTarget, DialogOpen };
enum class HasSkillRequirement {OnBar, OffCooldown, ReadyToUse};
enum class PlayerConnectednessRequirement {All, Individual};

enum class ActionBehaviourFlag : uint32_t 
{
    Default = 0,

    ImmediateFinish = 1 << 0,
    CanBeRunInOutpost = 1 << 1,

    All = 0xFFFF,
};

class ActionBehaviourFlags 
{
public:
    ActionBehaviourFlags() = default;
    ActionBehaviourFlags(ActionBehaviourFlag flag) { flags = (uint32_t)flag; }

    bool test(ActionBehaviourFlag flag) { return flags & (uint32_t)flag; }
    ActionBehaviourFlags& operator|(ActionBehaviourFlag flag)
    {
        flags |= (uint32_t)flag;
        return *this;
    }
    ActionBehaviourFlags& operator&=(ActionBehaviourFlags other) { 
        flags &= other.flags;
        return *this;
    }
    
private:
    uint32_t flags = 0;
};
inline ActionBehaviourFlags operator|(ActionBehaviourFlag a, ActionBehaviourFlag b) 
{
    return ActionBehaviourFlags{} | a | b;
}