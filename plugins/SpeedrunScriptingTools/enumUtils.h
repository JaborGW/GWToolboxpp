#pragma once

#include <Enums.h>

#include <string_view>
#include <vector>

#include <GWCA/GameContainers/GamePos.h>

namespace GW::Constants 
{
    enum class SkillID : uint32_t;
    enum class MapID : uint32_t;
    enum class InstanceType;
    enum HeroID : uint32_t;
}

std::string_view toString(AgentType);
std::string_view toString(Sorting);
std::string_view toString(AnyNoYes);
std::string_view toString(Class);
std::string_view toString(Channel);
std::string_view toString(QuestStatus);
std::string_view toString(GoToTargetFinishCondition);
std::string_view toString(HasSkillRequirement);
std::string_view toString(PlayerConnectednessRequirement);
std::string_view toString(Status);
std::string_view toString(EquippedItemSlot);
std::string_view toString(TrueFalse);
std::string_view toString(MoveToBehaviour);
std::string_view toString(Trigger type);
std::string_view toString(GW::Constants::InstanceType);
std::string_view toString(GW::Constants::HeroID);

std::string WStringToString(const std::wstring_view str);

bool pointIsInsidePolygon(const GW::GamePos pos, const std::vector<GW::Vec2f>& polygon);
