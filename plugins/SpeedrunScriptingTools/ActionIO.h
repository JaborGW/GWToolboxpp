#pragma once

#include <Action.h>
#include <memory>

std::shared_ptr<Action> readAction(InputStream& stream);
void drawActionSequenceSelector(std::vector<std::shared_ptr<Action>>& actions, std::optional<float> width = std::nullopt, bool showAddButton = true);
std::string_view toString(ActionType type);
