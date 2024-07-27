#pragma once

#include <enumUtils.h>
#include <Action.h>
#include <Condition.h>

#include <imgui.h>
#include <vector>
#include <string>
#include <optional>
#include <functional>

std::string makeHotkeyDescription(Hotkey key);

void drawSelector(int& value);
void drawSelector(float& value);
void drawSelector(Hotkey& hotkey, std::string& description, float selectorWidth);
void drawSelector(GW::Constants::SkillID& id);
void drawSelector(GW::Constants::MapID& id);
void drawSelector(uint16_t& id, std::string_view label);
void drawSelector(Trigger& trigger, float width, Hotkey& hotkey);
void drawSelector(GW::Vec2f& pos);
void drawSelector(std::vector<GW::Vec2f>& polygon, std::function<void()> drawButtons = []{}, std::optional<float> width = std::nullopt);

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
concept takesButtons = requires(T& t) {
    {
        drawSelector(t, std::function<void()>{})
    } -> std::same_as<void>;
};

template <typename T>
    requires(takesButtons<std::decay_t<T>>)
void drawSelectorImpl(T& value, std::function<void()> drawButtons)
{
    drawSelector(value, drawButtons);
}

template <typename T>
void drawSelectorImpl(T& value, std::function<void()> drawButtons)
{
    drawSelector(value);
    drawButtons();
}

template <typename T>
void drawListSelector(std::vector<T>& values, const std::string& name, std::optional<float> width = std::nullopt, std::function<void(int)> customDeleter = nullptr, std::function<void()> customAdder = nullptr)
{
    std::optional<int> rowToDelete;
    std::optional<std::pair<int, int>> rowsToSwap;

    for (int i = 0; i < int(values.size()); ++i) {
        ImGui::PushID(i);

        const auto drawButtons = [i, size = values.size(), &rowToDelete, &rowsToSwap] 
        {
            //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetTextLineHeight() - ImGui::GetStyle().FramePadding.x - 128.f);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80.f);
            if (ImGui::Button("X", ImVec2(20, 0))) rowToDelete = i;
            ImGui::SameLine();
            if (ImGui::Button("^", ImVec2(20, 0)) && i > 0) rowsToSwap = {i - 1, i};
            ImGui::SameLine();
            if (ImGui::Button("v", ImVec2(20, 0)) && i + 1 < (int)size) rowsToSwap = {i, i + 1};
        };

        drawSelectorImpl(values[i], drawButtons);

        ImGui::PopID();
    }
    if (rowToDelete) values.erase(values.begin() + *rowToDelete);
    if (rowsToSwap) std::swap(*(values.begin() + rowsToSwap->first), *(values.begin() + rowsToSwap->second));

    if (customAdder)
        customAdder();
    else {
        if (width) {
            if (ImGui::Button(("Add " + name).data(), ImVec2(*width, 0))) values.push_back({});
        }
        else {
            if (ImGui::Button(("Add " + name).data())) values.push_back({});
        }
    }
}

template <typename T>
void drawCollapsingListSelector(std::vector<T>& values, const std::vector<std::string_view>& names, const std::string& typeName, std::optional<float> width = std::nullopt, std::function<void(int)> customDeleter = nullptr, std::function<void()> customAdder = nullptr)
{
    std::optional<int> rowToDelete;
    std::optional<std::pair<int, int>> rowsToSwap;

    for (int i = 0; i < int(values.size()); ++i) {
        ImGui::PushID(i);

        const auto drawButtons = [i, size = values.size(), &rowToDelete, &rowsToSwap] {
            // ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetTextLineHeight() - ImGui::GetStyle().FramePadding.x - 128.f);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80.f);
            if (ImGui::Button("X", ImVec2(20, 0))) rowToDelete = i;
            ImGui::SameLine();
            if (ImGui::Button("^", ImVec2(20, 0)) && i > 0) rowsToSwap = {i - 1, i};
            ImGui::SameLine();
            if (ImGui::Button("v", ImVec2(20, 0)) && i + 1 < (int)size) rowsToSwap = {i, i + 1};
        };

        const auto treeOpen = ImGui::TreeNodeEx(names[i].data(), ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth);
        drawButtons();
        if (treeOpen) 
        {
            drawSelector(values[i], []{}, width);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
    if (rowToDelete) values.erase(values.begin() + *rowToDelete);
    if (rowsToSwap) std::swap(*(values.begin() + rowsToSwap->first), *(values.begin() + rowsToSwap->second));

    if (customAdder)
        customAdder();
    else {
        if (width) {
            if (ImGui::Button(("Add " + typeName).data(), ImVec2(*width, 0))) values.push_back({});
        }
        else {
            if (ImGui::Button(("Add " + typeName).data())) values.push_back({});
        }
    }
}
