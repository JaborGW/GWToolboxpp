#pragma once

#include <imgui.h>
#include <Enums.h>

#include <string_view>
#include <vector>
#include <optional>

#include <GWCA/GameContainers/GamePos.h>

namespace GW::Constants 
{
    enum class SkillID : uint32_t;
    enum class MapID : uint32_t;
    enum class InstanceType;
    enum HeroID : uint32_t;
} // namespace GW::Constants

namespace GW::UI 
{
    enum ControlAction : uint32_t;
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
std::string_view toString(ReferenceFrame);
std::string_view toString(GW::Constants::InstanceType);
std::string_view toString(GW::Constants::HeroID);
std::string_view toString(GW::UI::ControlAction);
std::string_view toString(WeaponType);
std::string_view toString(Bag);
std::string_view toString(ComparisonOperator);
std::string_view toString(IsIsNot);

bool checkWeaponType(WeaponType, uint16_t);
void drawHotkeySelector(Hotkey& hotkey, std::string& description, float selectorWidth);
std::string makeHotkeyDescription(Hotkey);
std::string getSkillName(GW::Constants::SkillID id, bool zeroIsAny);

void drawSkillIDSelector(GW::Constants::SkillID& id, bool zeroIsAny = false);
void drawMapIDSelector(GW::Constants::MapID& id);
void drawModelIDSelector(uint16_t& id, std::optional<std::string_view> label = std::nullopt);
void drawTriggerSelector(Trigger& trigger, TriggerData& triggerData, float width);

void drawPolygonSelector(std::vector<GW::Vec2f>& polygon);
bool pointIsInsidePolygon(const GW::GamePos pos, const std::vector<GW::Vec2f>& polygon);

template <typename T>
void drawEnumButton(T firstValue, T lastValue, T& currentValue, int id = 0, float width = 100., std::optional<std::string_view> buttonText = std::nullopt, std::optional<T> skipValue = std::nullopt)
{
    ImGui::PushID(id);
    using UnderlyingT = std::underlying_type_t<T>;

    if (ImGui::Button((buttonText ? buttonText.value() : toString(currentValue)).data(), ImVec2(width, 0))) 
    {
        ImGui::OpenPopup("Enum popup");
    }
    if (ImGui::BeginPopup("Enum popup")) 
    {
        for (auto i = (UnderlyingT)firstValue; i <= (UnderlyingT)lastValue; ++i) 
        {
            if (skipValue && (UnderlyingT)skipValue.value() == i) 
                continue;

            if (ImGui::Selectable(toString((T)i).data()))
                currentValue = (T)i;
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();
}

template<typename T>
bool compare(const T& a, ComparisonOperator comp, const T& b)
{
    switch (comp) {
        case ComparisonOperator::Equals:
            return a == b;
        case ComparisonOperator::Less:
            return a < b;
        case ComparisonOperator::Greater:
            return a > b;
        case ComparisonOperator::LessOrEqual:
            return a <= b;
        case ComparisonOperator::GreaterOrEqual:
            return a >= b;
        case ComparisonOperator::NotEquals:
            return a != b;
    }
    return false;
}

template <typename T>
bool compare(const T& a, IsIsNot comp, const T& b)
{
    switch (comp) 
    {
        case IsIsNot::Is:
            return a == b;
        case IsIsNot::IsNot:
            return a != b;
    }
    return false;
}

std::string WStringToString(const std::wstring_view str);