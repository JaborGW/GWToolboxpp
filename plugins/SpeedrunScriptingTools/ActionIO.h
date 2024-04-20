#pragma once

#include <Action.h>
#include <memory>

std::shared_ptr<Action> readAction(std::istringstream& stream);
std::shared_ptr<Action> drawActionSelector(float width);