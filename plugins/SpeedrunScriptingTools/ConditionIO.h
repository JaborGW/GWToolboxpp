#pragma once

#include <Condition.h>
#include <memory>

std::shared_ptr<Condition> readCondition(InputStream& stream);
std::shared_ptr<Condition> drawConditionSelector(float width);
void drawConditionSetSelector(std::vector<std::shared_ptr<Condition>>& conditions, std::optional<float> width = std::nullopt, bool showAddButton = true);
