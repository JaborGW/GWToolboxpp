#pragma once

#include <Action.h>
#include <memory>

std::shared_ptr<Action> readAction(InputStream& stream);
void drawSelector(std::shared_ptr<Action>& result, std::function<void()> drawButtons, std::optional<float> width = std::nullopt);
void drawSelector(std::vector<std::shared_ptr<Action>>& actions, std::optional<float> width = std::nullopt);
void drawCollapsingSelector(std::vector<std::shared_ptr<Action>>& actions, const std::vector<std::string_view>& names, std::optional<float> width = std::nullopt);
std::string_view toString(ActionType type);
