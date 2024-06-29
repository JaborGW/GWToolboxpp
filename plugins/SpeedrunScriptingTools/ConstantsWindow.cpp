#include <ConstantsWindow.h>
#include <Constants.h>

#include <imgui.h>
#include <ImGuiCppWrapper.h>
#include <ActionIO.h>
#include <ConditionIO.h>
#include <enumUtils.h>
#include <Selectors.h>

template<typename ConstantT>
void drawSection(int id)
{
    ImGui::PushID(id);
    auto& constantsManager = ConstantsManager::getInstance();
    auto& constants = constantsManager.list<ConstantT>();
    for (auto& constant : constants) 
    {
        ImGui::PushID(constant.id);
        
        ImGui::Bullet();
        ImGui::InputText("Name",&constant.name);
        ImGui::SameLine();
        ImGui::PushItemWidth(150.f);
        //drawSelector(constant.value);

        ImGui::PopID();
    }

    if(ImGui::Button("Add named constant"))
    {
        constantsManager.addNew<ConstantT>();
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
    drawSection<int>(id++);
    drawSection<float>(id++);
    drawSection<ConstantsManager::Position>(id++);
    drawSection<ConstantsManager::Polygon>(id++);
    drawSection<ConstantsManager::ActionSequence>(id++);
    drawSection<ConstantsManager::ConditionSet>(id++);


    ImGui::End();
}
