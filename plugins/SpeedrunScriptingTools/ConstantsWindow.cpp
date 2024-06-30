#include <ConstantsWindow.h>
#include <Constants.h>

#include <imgui.h>
#include <ImGuiCppWrapper.h>
#include <ActionIO.h>
#include <ConditionIO.h>
#include <enumUtils.h>
#include <Selectors.h>

template<typename T>
void drawSelector(NamedConstant<T>& constant)
{
    drawSelector(constant.value);
    ImGui::SameLine();
    ImGui::InputText("Name", &constant.name);
}

template<typename ConstantT>
void drawSection(int id, std::string_view name)
{
    ImGui::PushID(id);
    const auto groupName = std::string{name} + "s";
    if (ImGui::TreeNodeEx(groupName.c_str(), ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        auto& constantsManager = ConstantsManager::getInstance();
        auto& constants = constantsManager.list<ConstantT>();
        drawListSelector(constants, name.data());

        ImGui::TreePop();
    }
    ImGui::PopID();
}

template <typename ConstantT>
void drawCollapsingSection(int id, std::string_view name)
{
    ImGui::PushID(id);
    const auto groupName = std::string{name} + "s";
    if (ImGui::TreeNodeEx(groupName.c_str(), ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth)) {
        auto& constantsManager = ConstantsManager::getInstance();
        auto& constants = constantsManager.list<ConstantT>();
        drawListSelector(constants, name.data());

        ImGui::TreePop();
    }
    ImGui::PopID();
}

void drawConstantsWindow(bool& showWindow)
{
    if (!ImGui::Begin("Constants Manager", &showWindow)) 
    {
        ImGui::End();
        return;
    }
    
    int id = 0;
    ImGui::PushItemWidth(100.f);
    drawSection<int>(id++, "Integer constant");
    drawSection<float>(id++, "Float constant");
    drawSection<ConstantsManager::Position>(id++, "Position constant");
    drawSection<ConstantsManager::Polygon>(id++, "Polygon constant");
    drawSection<ConstantsManager::ActionSequence>(id++, "Action sequence");
    drawSection<ConstantsManager::ConditionSet>(id++, "Condition set");


    ImGui::End();
}
