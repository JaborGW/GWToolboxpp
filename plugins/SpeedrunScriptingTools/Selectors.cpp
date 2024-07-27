#include <Selectors.h>

#include <commonIncludes.h>

#include <GWCA/Constants/Skills.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Managers/UIMgr.h>

#include <Keys.h>
#include <ModelNames.h>


namespace {
    std::string getSkillName(GW::Constants::SkillID id)
    {
        static std::unordered_map<GW::Constants::SkillID, std::wstring> decodedNames;
        if (const auto it = decodedNames.find(id); it != decodedNames.end()) {
            return WStringToString(it->second);
        }

        const auto skillData = GW::SkillbarMgr::GetSkillConstantData(id);
        if (!skillData || (uint32_t)id >= (uint32_t)GW::Constants::SkillID::Count) return "";

        wchar_t out[8] = {0};
        if (GW::UI::UInt32ToEncStr(skillData->name, out, _countof(out))) {
            GW::UI::AsyncDecodeStr(out, &decodedNames[id]);
        }
        return "";
    }
} // namespace

void drawSelector(int& value)
{
    ImGui::InputInt("", &value, 0);
}
void drawSelector(float& value)
{
    ImGui::InputFloat("", &value);
}
void drawSelector(GW::Vec2f& pos)
{
    ImGui::InputFloat("x", &pos.x);
    ImGui::SameLine();
    ImGui::InputFloat("y", &pos.y);
}

std::string makeHotkeyDescription(Hotkey key)
{
    char newDescription[256];
    ModKeyName(newDescription, _countof(newDescription), key.modifier, key.keyData);
    return std::string{newDescription};
}

void drawSelector(Hotkey& key, std::string& description, float selectorWidth)
{
    ImGui::PushItemWidth(selectorWidth);
    if (ImGui::Button(description.c_str())) {
        ImGui::OpenPopup("Select Hotkey");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to change hotkey");
    if (ImGui::BeginPopup("Select Hotkey")) {
        static long newkey = 0;
        char newkey_buf[256]{};
        long newmod = 0;

        ImGui::Text("Press key");
        if (ImGui::IsKeyDown(ImGuiKey_ModCtrl)) newmod |= ModKey_Control;
        if (ImGui::IsKeyDown(ImGuiKey_ModShift)) newmod |= ModKey_Shift;
        if (ImGui::IsKeyDown(ImGuiKey_ModAlt)) newmod |= ModKey_Alt;

        if (newkey == 0) { // we are looking for the key
            for (WORD i = 0; i < 512; i++) {
                switch (i) {
                    case VK_CONTROL:
                    case VK_LCONTROL:
                    case VK_RCONTROL:
                    case VK_SHIFT:
                    case VK_LSHIFT:
                    case VK_RSHIFT:
                    case VK_MENU:
                    case VK_LMENU:
                    case VK_RMENU:
                    case VK_LBUTTON:
                    case VK_RBUTTON:
                        continue;
                    default: {
                        if (::GetKeyState(i) & 0x8000) newkey = i;
                    }
                }
            }
        }
        else if (!(::GetKeyState(newkey) & 0x8000)) { // We have a key, check if it was released
            key.keyData = newkey;
            key.modifier = newmod;
            description = makeHotkeyDescription(key);
            newkey = 0;
            ImGui::CloseCurrentPopup();
        }

        ModKeyName(newkey_buf, _countof(newkey_buf), newmod, newkey);
        ImGui::Text("%s", newkey_buf);

        ImGui::EndPopup();
    }
}

void drawSelector(Trigger& trigger, float width, Hotkey& hotkey)
{
    if (trigger == Trigger::None) {
        drawEnumButton(Trigger::InstanceLoad, Trigger::Hotkey, trigger, 0, 100.f, "Add trigger");
    }
    else if (trigger == Trigger::Hotkey) {
        auto description = hotkey.keyData ? makeHotkeyDescription(hotkey) : "Click to change key";
        drawSelector(hotkey, description, width - 20.f);
        ImGui::SameLine();
        ImGui::Text("Hotkey");
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20.f, 0))) {
            trigger = Trigger::None;
            hotkey = {};
        }
    }
    else {
        ImGui::Text(toString(trigger).data());
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20.f, 0))) {
            trigger = Trigger::None;
        }
    }
}

void drawSelector(std::vector<GW::Vec2f>& polygon, std::function<void()> drawButtons, std::optional<float> width)
{
    ImGui::PushItemWidth(width.value_or(200.f));
    if (ImGui::Button("Add Polygon Point")) {
        if (const auto player = GW::Agents::GetPlayerAsAgentLiving()) {
            polygon.emplace_back(player->pos.x, player->pos.y);
        }
    }
    drawButtons();

    ImGui::Indent();

    std::optional<int> remove_point;
    for (auto j = 0u; j < polygon.size(); j++) {
        ImGui::PushID(j);
        ImGui::Bullet();
        ImGui::InputFloat2("", reinterpret_cast<float*>(&polygon.at(j)));
        ImGui::SameLine();
        if (ImGui::Button("x")) remove_point = j;
        ImGui::PopID();
    }
    if (remove_point) {
        polygon.erase(polygon.begin() + remove_point.value());
    }

    ImGui::Unindent();
}

void drawSelector(GW::Constants::SkillID& id)
{
    ImGui::PushItemWidth(50.f);
    if (id != GW::Constants::SkillID::No_Skill) {
        ImGui::Text("%s", getSkillName(id).c_str());
        ImGui::SameLine();
    }
    ImGui::SameLine();
    ImGui::InputInt("Skill ID", reinterpret_cast<int*>(&id), 0);
}

void drawSelector(GW::Constants::MapID& id)
{
    ImGui::PushItemWidth(50.f);
    if (id != GW::Constants::MapID::None && (uint32_t)id < GW::Constants::NAME_FROM_ID.size()) {
        ImGui::Text("%s", GW::Constants::NAME_FROM_ID[(uint32_t)id]);
        ImGui::SameLine();
    }

    ImGui::InputInt("Map ID", reinterpret_cast<int*>(&id), 0);
}
void drawSelector(uint16_t& id, std::string_view label)
{
    ImGui::PushItemWidth(50.f);
    const auto& modelNames = getModelNames();
    const auto& modelNameIt = modelNames.find(id);
    if (modelNameIt != modelNames.end()) {
        ImGui::Text("%s", modelNameIt->second.data());
        ImGui::SameLine();
    }
    int editValue = id;

    if (ImGui::InputInt(label.data(), &editValue, 0)) {
        if (editValue >= 0 && editValue <= 0xFFFF)
            id = uint16_t(editValue);
        else
            id = 0;
    }
}
