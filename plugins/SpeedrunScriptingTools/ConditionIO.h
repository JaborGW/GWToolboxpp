#pragma once

#include <Condition.h>
#include <memory>

std::shared_ptr<Condition> readCondition(InputStream& stream);

void drawSelector(std::shared_ptr<Condition>&, std::function<void()> drawButtons, std::optional<float> width = std::nullopt);
void drawSelector(std::vector<std::shared_ptr<Condition>>& conditions, std::optional<float> width = std::nullopt);
