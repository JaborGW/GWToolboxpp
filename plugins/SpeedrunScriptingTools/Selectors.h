#pragma once

#include <enumUtils.h>

#include <imgui.h>
#include <vector>
#include <string>
#include <optional>
std::string makeHotkeyDescription(Hotkey key);

void drawSelector(Hotkey& hotkey, std::string& description, float selectorWidth);
void drawSelector(GW::Constants::SkillID& id);
void drawSelector(GW::Constants::MapID& id);
void drawSelector(uint16_t& id, std::optional<std::string_view> label = std::nullopt);
void drawSelector(Trigger& trigger, float width, Hotkey& hotkey);
void drawSelector(std::vector<GW::Vec2f>& polygon);

template <typename T>
void drawEnumButton(T firstValue, T lastValue, T& currentValue, int id = 0, float width = 100., std::optional<std::string_view> buttonText = std::nullopt, std::optional<T> skipValue = std::nullopt)
{
    ImGui::PushID(id);
    using UnderlyingT = std::underlying_type_t<T>;

    if (ImGui::Button((buttonText ? buttonText.value() : toString(currentValue)).data(), ImVec2(width, 0))) {
        ImGui::OpenPopup("Enum popup");
    }
    if (ImGui::BeginPopup("Enum popup")) {
        for (auto i = (UnderlyingT)firstValue; i <= (UnderlyingT)lastValue; ++i) {
            if (skipValue && (UnderlyingT)skipValue.value() == i) continue;

            if (ImGui::Selectable(toString((T)i).data())) currentValue = (T)i;
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();
}

template <typename T>
void drawListSelector(std::vector<T>& values, const std::string& name, std::optional<float> width = std::nullopt)
{
    std::optional<int> rowToDelete;
    std::optional<std::pair<int, int>> rowsToSwap;

    for (int i = 0; i < int(values.size()); ++i) {
        ImGui::PushID(i);

        if (ImGui::Button("X", ImVec2(20, 0)))
            rowToDelete = i;
        ImGui::SameLine();
        if (ImGui::Button("^", ImVec2(20, 0)) && i > 0) rowsToSwap = {i - 1, i};
        ImGui::SameLine();
        if (ImGui::Button("v", ImVec2(20, 0)) && i + 1 < int(values.size())) rowsToSwap = {i, i + 1};

        ImGui::SameLine();
        drawSelector(values[i]);

        ImGui::PopID();
    }
    if (rowToDelete) values.erase(values.begin() + *rowToDelete);
    if (rowsToSwap) std::swap(*(values.begin() + rowsToSwap->first), *(values.begin() + rowsToSwap->second));

    if (width) 
    {
        if (ImGui::Button(("Add " + name).data(), ImVec2(*width, 0))) values.push_back({});
    }
    else 
    {
        if (ImGui::Button(("Add " + name).data())) values.push_back({});
    }
}
