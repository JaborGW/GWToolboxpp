#include <enumUtils.h>

#include <commonIncludes.h>
#include <SkillNames.h>
#include <Keys.h>

#include <GWCA/Constants/Skills.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/AgentMgr.h>

std::string_view toString(Status status)
{
    switch (status) 
    {
        case Status::Any:
            return "Any";
        case Status::Dead:
            return "Dead";
        case Status::Alive:
            return "Alive";
    }
    return "";
}

std::string_view toString(Class c)
{
    switch (c) {
        case Class::Warrior:
            return "Warrior";
        case Class::Ranger:
            return "Ranger";
        case Class::Monk:
            return "Monk";
        case Class::Necro:
            return "Necro";
        case Class::Mesmer:
            return "Mesmer";
        case Class::Elementalist:
            return "Elementalist";
        case Class::Assassin:
            return "Assassin";
        case Class::Ritualist:
            return "Ritualist";
        case Class::Paragon:
            return "Paragon";
        case Class::Dervish:
            return "Dervish";
        case Class::Any:
            return "Any";
    }
    return "";
}
std::string_view toString(AgentType type) {
    switch (type) {
        case AgentType::Any:
            return "Any";
        case AgentType::Self:
            return "Self";
        case AgentType::PartyMember:
            return "Party member";
        case AgentType::Friendly:
            return "Friendly";
        case AgentType::Hostile:
            return "Hostile";
    }
    return "";
}
std::string_view toString(Sorting sorting)
{
    switch (sorting) {
        case Sorting::AgentId:
            return "Any";
        case Sorting::ClosestToPlayer:
            return "Closest to player";
        case Sorting::FurthestFromPlayer:
            return "Furthest from player";
        case Sorting::ClosestToTarget:
            return "Closest to target";
        case Sorting::FurthestFromTarget:
            return "Furthest from target";
        case Sorting::LowestHp:
            return "Lowest HP";
        case Sorting::HighestHp:
            return "Highest HP";
    }
    return "";
}
std::string_view toString(HexedStatus status) {
    switch (status) {
        case HexedStatus::Any:
            return "Any";
        case HexedStatus::NotHexed:
            return "Not hexed";
        case HexedStatus::Hexed:
            return "Hexed";
    }
    return "";
}
std::string_view toString(Channel channel)
{
    switch (channel) {
        case Channel::All:
            return "All";
        case Channel::Guild:
            return "Guild";
        case Channel::Team:
            return "Team";
        case Channel::Trade:
            return "Trade";
        case Channel::Alliance:
            return "Alliance";
        case Channel::Whisper:
            return "Whisper";
        case Channel::Emote:
            return "Emote";
    }
    return "";
}
std::string_view toString(QuestStatus status)
{
    switch (status) {
        case QuestStatus::NotStarted:
            return "Not started";
        case QuestStatus::Started:
            return "Started";
        case QuestStatus::Completed:
            return "Completed";
        case QuestStatus::Failed:
            return "Failed";
    }
    return "";
}

std::string_view toString(GoToTargetFinishCondition cond)
{
    switch (cond) {
        case GoToTargetFinishCondition::None:
            return "Immediately";
        case GoToTargetFinishCondition::StoppedMovingNextToTarget:
            return "When next to target and not moving";
        case GoToTargetFinishCondition::DialogOpen:
            return "When dialog has opened";
    }
    return "";
}

std::string_view toString(HasSkillRequirement req)
{
    switch (req) {
        case HasSkillRequirement::OnBar:
            return "On the skillbar";
        case HasSkillRequirement::OffCooldown:
            return "Off cooldown";
        case HasSkillRequirement::ReadyToUse:
            return "Ready to use";
    }
    return "";
}

std::string_view toString(Trigger type)
{
    switch (type) {
        case Trigger::None:
            return "No packet trigger";
        case Trigger::InstanceLoad:
            return "Trigger on instance load";
        case Trigger::HardModePing:
            return "Trigger on hard mode ping";
        case Trigger::Hotkey:
            return "Trigger on keypress";
    }
    return "";
}

std::string makeHotkeyDescription(long keyData, long modifier) 
{
    char newDescription[256];
    ModKeyName(newDescription, _countof(newDescription), modifier, keyData);
    return std::string{newDescription};
}

void drawHotkeySelector(long& keyData, long& modifier, std::string& description, float selectorWidth) 
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
            keyData = newkey;
            modifier = newmod;
            description = makeHotkeyDescription(newkey, newmod);
            newkey = 0;
            ImGui::CloseCurrentPopup();
        }

        ModKeyName(newkey_buf, _countof(newkey_buf), newmod, newkey);
        ImGui::Text("%s", newkey_buf);

        ImGui::EndPopup();
    }
}

void drawTriggerSelector(Trigger& trigger, float width, long& hotkeyData, long& hotkeyMod)
{
    if (trigger == Trigger::None) 
    {
        drawEnumButton(Trigger::InstanceLoad, Trigger::Hotkey, trigger, 0, 100.f, "Add trigger");
    }
    else if (trigger == Trigger::Hotkey) 
    {
        auto description = hotkeyData ? makeHotkeyDescription(hotkeyData, hotkeyMod) : "Click to change key";
        drawHotkeySelector(hotkeyData, hotkeyMod, description, width - 20.f);
        ImGui::SameLine();
        ImGui::Text("Hotkey");
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20.f, 0))) {
            trigger = Trigger::None;
            hotkeyData = 0;
            hotkeyMod = 0;
        }
    }
    else 
    {
        ImGui::Text(toString(trigger).data());
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20.f, 0))) {
            trigger = Trigger::None;
        }
    }
}

void drawPolygonSelector(std::vector<GW::Vec2f>& polygon)
{
    ImGui::PushItemWidth(200);
    if (ImGui::Button("Add Polygon Point")) {
        if (const auto player = GW::Agents::GetPlayerAsAgentLiving()) {
            polygon.emplace_back(player->pos.x, player->pos.y);
        }
    }
    
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
bool pointIsInsidePolygon(const GW::GamePos pos, const std::vector<GW::Vec2f>& points)
{
    bool b = false;
    for (auto i = 0u, j = points.size() - 1; i < points.size(); j = i++) {
        if (points[i].y >= pos.y != points[j].y >= pos.y && pos.x <= (points[j].x - points[i].x) * (pos.y - points[i].y) / (points[j].y - points[i].y) + points[i].x) {
            b = !b;
        }
    }
    return b;
}

void drawSkillIDSelector(GW::Constants::SkillID& id)
{
    ImGui::PushItemWidth(50.f);
    const auto& skillNames = getSkillNames();
    if (id != GW::Constants::SkillID::No_Skill && (uint32_t)id < skillNames.size()) {
        ImGui::Text("%s", skillNames[(uint32_t)id]);
        ImGui::SameLine();
    }
    ImGui::SameLine();
    ImGui::InputInt("Skill ID", reinterpret_cast<int*>(&id), 0);
}

void drawMapIDSelector(GW::Constants::MapID& id) 
{
    ImGui::PushItemWidth(50.f);
    if (id != GW::Constants::MapID::None && (uint32_t)id < GW::Constants::NAME_FROM_ID.size()) 
    {
        ImGui::Text("%s", GW::Constants::NAME_FROM_ID[(uint32_t)id]);
        ImGui::SameLine();
    }
    
    ImGui::InputInt("Map ID", reinterpret_cast<int*>(&id), 0);
}
