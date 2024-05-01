#include <ConditionImpls.h>

#include <ConditionIO.h>
#include <utils.h>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/CharContext.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/ItemMgr.h>

#include <Keys.h>

#include "windows.h"
#include "imgui.h"
#include "ImGuiCppWrapper.h"

#include <algorithm>
#include <optional>

namespace {
    constexpr double eps = 1e-3;
    constexpr float indent = 30.f;
    const std::string missingContentToken = "/";
    const std::string endOfListToken = ">";

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

    template <typename T>
    void drawEnumButton(T firstValue, T lastValue, T& currentValue, int id = 0, float width = 100.)
    {
        ImGui::PushID(id);

        if (ImGui::Button(toString(currentValue).data(), ImVec2(width, 0))) {
            ImGui::OpenPopup("Enum popup");
        }
        if (ImGui::BeginPopup("Enum popup")) {
            for (auto i = (int)firstValue; i <= (int)lastValue; ++i) {
                if (ImGui::Selectable(toString((T)i).data())) {
                    currentValue = static_cast<T>(i);
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    GW::Item* FindMatchingItem(GW::Constants::Bag _bag_idx, uint32_t model_id)
    {
        GW::Bag* bag = GW::Items::GetBag(_bag_idx);
        if (!bag) return nullptr;
        GW::ItemArray& items = bag->items;
        for (auto _item : items) {
            if (_item && _item->model_id == model_id) return _item;
        }
        return nullptr;
    }
}

/// ------------- InvertedCondition -------------
NegatedCondition::NegatedCondition(std::istringstream& stream)
{
    std::string read;
    stream >> read;
    if (read == missingContentToken) {
        cond = nullptr;
    }
    else if (read == "C"){
        cond = readCondition(stream);
    }
    else
    {
        assert(false);
    }
}
void NegatedCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);
    if (cond)
        cond->serialize(stream);
    else
        stream << missingContentToken << " ";
}
bool NegatedCondition::check() const
{
    return cond && !cond->check();
}
void NegatedCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("NOT (");
    ImGui::SameLine();
    if (cond) {
        cond->drawSettings();
    }
    else {
        cond = drawConditionSelector(100.f);
    }
    ImGui::SameLine();
    ImGui::Text(")");
    ImGui::PopID();
}

/// ------------- DisjunctionCondition -------------
DisjunctionCondition::DisjunctionCondition(std::istringstream& stream) 
{
    std::string read;
    
    while (stream >> read) {
        
        if (read == endOfListToken)
            return;
        else if (read == missingContentToken)
            conditions.push_back(nullptr);
        else if (read == "C")
            conditions.push_back(readCondition(stream));
        else
            assert(false);
    }
}
void DisjunctionCondition::serialize(std::ostringstream& stream) const 
{
    Condition::serialize(stream);

    for (const auto& condition : conditions) {
        if (condition)
            condition->serialize(stream);
        else
            stream << missingContentToken << " ";
    }
    stream << endOfListToken << " ";
}
bool DisjunctionCondition::check() const
{
    return std::ranges::any_of(conditions, [](const auto& condition) {return condition && condition->check();});
}
void DisjunctionCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If ONE of the following is true:");
    ImGui::Indent(indent);

    int rowToDelete = -1;
    for (int i = 0; i < int(conditions.size()); ++i) {
        ImGui::PushID(i);

        ImGui::Bullet();
        if (ImGui::Button("X")) {
            if (conditions[i])
                conditions[i] = nullptr;
            else
                rowToDelete = i;
        }

        ImGui::SameLine();
        if (conditions[i])
            conditions[i]->drawSettings();
        else
            conditions[i] = drawConditionSelector(100.f);

        ImGui::PopID();
    }
    if (rowToDelete != -1) conditions.erase(conditions.begin() + rowToDelete);

    ImGui::Bullet();
    if (ImGui::Button("+")) {
        conditions.push_back(nullptr);
    }

    ImGui::Unindent(indent);
    ImGui::PopID();
}

/// ------------- ConjunctionCondition -------------
ConjunctionCondition::ConjunctionCondition(std::istringstream& stream)
{
    std::string read;

    while (stream >> read) {
        if (read == endOfListToken)
            return;
        else if (read == missingContentToken)
            conditions.push_back(nullptr);
        else if (read == "C")
            conditions.push_back(readCondition(stream));
        else
            assert(false);
    }
}
void ConjunctionCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    for (const auto& condition : conditions) {
        if (condition)
            condition->serialize(stream);
        else
            stream << missingContentToken << " ";
    }
    stream << endOfListToken << " ";
}
bool ConjunctionCondition::check() const
{
    return std::ranges::all_of(conditions, [](const auto& condition) {return !condition || condition->check();});
}
void ConjunctionCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If ALL of the following is true:");
    ImGui::Indent(indent);

    int rowToDelete = -1;
    for (int i = 0; i < int(conditions.size()); ++i) {
        ImGui::PushID(i);

        ImGui::Bullet();
        if (ImGui::Button("X")) {
            if (conditions[i])
                conditions[i] = nullptr;
            else
                rowToDelete = i;
        }

        ImGui::SameLine();
        if (conditions[i])
            conditions[i]->drawSettings();
        else
            conditions[i] = drawConditionSelector(100.f);

        ImGui::PopID();
    }
    if (rowToDelete != -1) conditions.erase(conditions.begin() + rowToDelete);

    ImGui::Bullet();
    if (ImGui::Button("+")) {
        conditions.push_back(nullptr);
    }

    ImGui::Unindent(indent);
    ImGui::PopID();
}

/// ------------- IsInMapCondition -------------
IsInMapCondition::IsInMapCondition(std::istringstream& stream)
{
    stream >> id;
}
void IsInMapCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << id << " ";
}
bool IsInMapCondition::check() const {
    return GW::Map::GetMapID() == id;
}
void IsInMapCondition::drawSettings() {
    ImGui::PushID(drawId());
    ImGui::Text("If player is in map");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    ImGui::InputInt("id", reinterpret_cast<int*>(&id), 0);
    ImGui::PopID();
}

/// ------------- PartyPlayerCountCondition -------------
PartyPlayerCountCondition::PartyPlayerCountCondition(std::istringstream& stream)
{
    stream >> count;
}
void PartyPlayerCountCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << count << " ";
}
bool PartyPlayerCountCondition::check() const
{
    return GW::PartyMgr::GetPartyPlayerCount() == uint32_t(count);
}
void PartyPlayerCountCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the number of players in the party is");
    ImGui::PushItemWidth(30);
    ImGui::SameLine();
    ImGui::InputInt("", &count, 0);
    ImGui::PopID();
}

/// ------------- InstanceProgressCondition -------------
InstanceProgressCondition::InstanceProgressCondition(std::istringstream& stream)
{
    stream >> requiredProgress;
}
void InstanceProgressCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << requiredProgress << " ";
}
bool InstanceProgressCondition::check() const {
    return GW::GetGameContext()->character->progress_bar->progress * 100.f > requiredProgress;
}
void InstanceProgressCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the instance progress is greater than");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    ImGui::InputFloat("%", &requiredProgress, 0);
    ImGui::PopID();
}

/// ------------- OnlyTriggerOnceCondition -------------
OnlyTriggerOnceCondition::OnlyTriggerOnceCondition(std::istringstream&)
{
}
bool OnlyTriggerOnceCondition::check() const
{
    const auto currentInstanceId = InstanceInfo::getInstance().getInstanceId();
    if (triggeredLastInInstanceId == currentInstanceId) return false;
    
    triggeredLastInInstanceId = currentInstanceId;
    return true;
}
void OnlyTriggerOnceCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If script has not been triggered in this instance");
    ImGui::PopID();
}

/// ------------- PlayerIsNearPositionCondition -------------
PlayerIsNearPositionCondition::PlayerIsNearPositionCondition(std::istringstream& stream)
{
    stream >> pos.x >> pos.y >> accuracy;
}
void PlayerIsNearPositionCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << pos.x << " " << pos.y << " " << accuracy << " ";
}
bool PlayerIsNearPositionCondition::check() const
{
    const auto player = GW::Agents::GetPlayerAsAgentLiving(); 
    if (!player) return false;
    return GW::GetDistance(player->pos, pos) < accuracy + eps;
}
void PlayerIsNearPositionCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the player is near position");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    ImGui::InputFloat("x", &pos.x, 0.0f, 0.0f);
    ImGui::SameLine();
    ImGui::InputFloat("y", &pos.y, 0.0f, 0.0f);
    ImGui::SameLine();
    ImGui::InputFloat("Accuracy", &accuracy, 0.0f, 0.0f);
    ImGui::PopID();
}

/// ------------- PlayerHasBuffCondition -------------
PlayerHasBuffCondition::PlayerHasBuffCondition(std::istringstream& stream)
{
    stream >> id >> minimumDuration >> maximumDuration;
}
void PlayerHasBuffCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << id << " " << minimumDuration << " " << maximumDuration << " ";
}
bool PlayerHasBuffCondition::check() const
{
    const auto effect = GW::Effects::GetPlayerEffectBySkillId(id);
    if (!effect) return false;
    if (minimumDuration > 0 && effect->GetTimeRemaining() < DWORD(minimumDuration)) return false;
    if (maximumDuration > 0 && effect->GetTimeRemaining() > DWORD(maximumDuration)) return false;
    return true;
}
void PlayerHasBuffCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the player is affected by the skill");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    ImGui::InputInt("id", reinterpret_cast<int*>(&id), 0);
    ImGui::SameLine();
    ImGui::Text("Min/Max duration in ms (0 for any):");
    ImGui::SameLine();
    ImGui::InputInt("min", &minimumDuration, 0);
    ImGui::SameLine();
    ImGui::InputInt("max", &maximumDuration, 0);
    ImGui::PopID();
}

/// ------------- PlayerHasSkillCondition -------------
PlayerHasSkillCondition::PlayerHasSkillCondition(std::istringstream& stream)
{
    stream >> id;
}
void PlayerHasSkillCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << id << " ";
}
bool PlayerHasSkillCondition::check() const
{
    GW::Skillbar* bar = GW::SkillbarMgr::GetPlayerSkillbar();
    if (!bar || !bar->IsValid()) return false;
    for (int i = 0; i < 8; ++i) {
        if (bar->skills[i].skill_id == id) {
            return bar->skills[i].GetRecharge() == 0;
        }
    }
    return false;
}
void PlayerHasSkillCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the player has the skill");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    ImGui::InputInt("id", reinterpret_cast<int*>(&id), 0);
    ImGui::SameLine();
    ImGui::Text("off cooldown");
    ImGui::PopID();
}

/// ------------- PlayerHasClassCondition -------------
PlayerHasClassCondition::PlayerHasClassCondition(std::istringstream& stream)
{
    stream >> primary >> secondary;
}
void PlayerHasClassCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << primary << " " << secondary << " ";
}
bool PlayerHasClassCondition::check() const
{
    const auto player = GW::Agents::GetPlayerAsAgentLiving();
    if (!player) return false;
    return (primary == Class::Any || primary == (Class)player->primary) && (secondary == Class::Any || secondary == (Class)player->secondary);
}
void PlayerHasClassCondition::drawSettings()
{
    ImGui::PushID(drawId());
    
    ImGui::PushID(0);
    ImGui::Text("If the player has primary");
    ImGui::SameLine();
    drawEnumButton(Class::Any, Class::Dervish, primary);
    ImGui::PopID();

    ImGui::SameLine();

    ImGui::PushID(1);
    ImGui::Text("and secondary");
    ImGui::SameLine();
    drawEnumButton(Class::Any, Class::Dervish, secondary);
    ImGui::PopID();

    ImGui::PopID();
}

/// ------------- PlayerHasNameCondition -------------
PlayerHasNameCondition::PlayerHasNameCondition(std::istringstream& stream)
{
    name = readStringWithSpaces(stream);
}
void PlayerHasNameCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    writeStringWithSpaces(stream, name);
}
bool PlayerHasNameCondition::check() const
{
    if (name.empty()) return false;
    const auto player = GW::Agents::GetPlayerAsAgentLiving();
    if (!player) return false;

    auto& instanceInfo = InstanceInfo::getInstance();
    return instanceInfo.getDecodedName(player->agent_id) == name;
}
void PlayerHasNameCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If player character has name");
    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    ImGui::InputText("name", &name);
    ImGui::PopID();
}

/// ------------- PlayerHasEnergyCondition -------------
PlayerHasEnergyCondition::PlayerHasEnergyCondition(std::istringstream& stream)
{
    stream >> minEnergy;
}
void PlayerHasEnergyCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << minEnergy << " ";
}
bool PlayerHasEnergyCondition::check() const
{
    const auto player = GW::Agents::GetPlayerAsAgentLiving();
    if (!player) return false;

    return player->energy * player->max_energy >= minEnergy;
}
void PlayerHasEnergyCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If player has at least");
    ImGui::SameLine();
    ImGui::PushItemWidth(90);
    ImGui::InputInt("energy", &minEnergy, 0);
    ImGui::PopID();
}

/// ------------- CurrentTargetIsCastingSkillCondition -------------
CurrentTargetIsCastingSkillCondition::CurrentTargetIsCastingSkillCondition(std::istringstream& stream)
{
    stream >> id;
}
void CurrentTargetIsCastingSkillCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << id << " ";
}
bool CurrentTargetIsCastingSkillCondition::check() const
{
    const auto target = GW::Agents::GetTargetAsAgentLiving();
    return target && static_cast<GW::Constants::SkillID>(target->skill) == id;
}
void CurrentTargetIsCastingSkillCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the target is casting the skill");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    ImGui::InputInt("id", reinterpret_cast<int*>(&id), 0);
    ImGui::PopID();
}

/// ------------- CurrentTargetHasHpBelowCondition -------------
CurrentTargetHasHpBelowCondition::CurrentTargetHasHpBelowCondition(std::istringstream& stream)
{
    stream >> hp;
}
void CurrentTargetHasHpBelowCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << hp << " ";
}
bool CurrentTargetHasHpBelowCondition::check() const
{
    const auto target = GW::Agents::GetTargetAsAgentLiving();
    return target && target->hp * 100.f < hp;
}
void CurrentTargetHasHpBelowCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the target has HP below");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    ImGui::InputFloat("%", &hp, 0);
    ImGui::PopID();
}

/// ------------- CurrentTargetAllegianceCondition -------------
CurrentTargetAllegianceCondition::CurrentTargetAllegianceCondition(std::istringstream& stream)
{
    stream >> agentType;
}
void CurrentTargetAllegianceCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << agentType << " ";
}
bool CurrentTargetAllegianceCondition::check() const
{
    const auto target = GW::Agents::GetTargetAsAgentLiving();
    if (!target) return false;
    switch (agentType) {
        case AgentType::Any:
            return true;
        case AgentType::PartyMember:
            return target->IsPlayer();
        case AgentType::Friendly:
            return target->allegiance != GW::Constants::Allegiance::Enemy;
        case AgentType::Hostile:
            return target->allegiance == GW::Constants::Allegiance::Enemy;
        default:
            return false;
    }
}
void CurrentTargetAllegianceCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the target has allegiance");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    drawEnumButton(AgentType::Any, AgentType::Hostile, agentType);
    ImGui::PopID();
}

/// ------------- CurrentTargetModelCondition -------------
CurrentTargetModelCondition::CurrentTargetModelCondition(std::istringstream& stream)
{
    stream >> modelId;
}
void CurrentTargetModelCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << modelId << " ";
}
bool CurrentTargetModelCondition::check() const
{
    const auto target = GW::Agents::GetTargetAsAgentLiving();
    return target && target->player_number == modelId;
}
void CurrentTargetModelCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the target has model");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    ImGui::InputInt("id", &modelId, 0);
    ImGui::PopID();
}

/// ------------- HasPartyWindowAllyOfNameCondition -------------
HasPartyWindowAllyOfNameCondition::HasPartyWindowAllyOfNameCondition(std::istringstream& stream)
{
    name = readStringWithSpaces(stream);
}
void HasPartyWindowAllyOfNameCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    writeStringWithSpaces(stream, name);
}
bool HasPartyWindowAllyOfNameCondition::check() const
{
    if (name.empty()) return false;
    const auto info = GW::PartyMgr::GetPartyInfo();
    const auto agentArray = GW::Agents::GetAgentArray();
    if (!info || !agentArray) return false;

    auto& instanceInfo = InstanceInfo::getInstance();

    for (const auto& player : info->players) {
        auto candidate = GW::Agents::GetAgentIdByLoginNumber(player.login_number);
        if (instanceInfo.getDecodedName(candidate) == name) { return true; }
    }

    return std::ranges::any_of(info->others, [&](const auto& allyId) {
        return instanceInfo.getDecodedName(allyId) == name;
    });
}
void HasPartyWindowAllyOfNameCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("Has party window ally of name");
    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    ImGui::InputText("Ally name", &name);
    ImGui::PopID();
}

/// ------------- PartyMemberStatusCondition -------------
PartyMemberStatusCondition::PartyMemberStatusCondition(std::istringstream& stream)
{
    stream >> status;
    name = readStringWithSpaces(stream);
}
void PartyMemberStatusCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << status;
    writeStringWithSpaces(stream, name);
}
bool PartyMemberStatusCondition::check() const
{
    if (name.empty()) return false;
    const auto info = GW::PartyMgr::GetPartyInfo();
    const auto agentArray = GW::Agents::GetAgentArray();
    if (!info || !agentArray) return false;

    auto& instanceInfo = InstanceInfo::getInstance();

    GW::Agents::GetMapAgentArray();

    for (const auto& [playerAgentId, decodedName] : instanceInfo.getPlayerNames()) 
    {
        if (decodedName != name) continue;

        if (status == Status::Any) return true; // Player is in the instance

        const auto mapAgent = GW::Agents::GetMapAgentByID(playerAgentId);
        if (!mapAgent) continue;
        return (mapAgent->GetIsDead()) == (status == Status::Dead);
    }

    return false;
}
void PartyMemberStatusCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("Party window ally status");
    ImGui::SameLine();
    drawEnumButton(Status::Any, Status::Alive, status);
    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    ImGui::InputText("Ally name", &name);
    ImGui::PopID();
}

/// ------------- QuestHasStateCondition -------------

QuestHasStateCondition::QuestHasStateCondition(std::istringstream& stream)
{
    stream >> id >> status;
}
void QuestHasStateCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << id << " " << status << " ";
}
bool QuestHasStateCondition::check() const
{
    return InstanceInfo::getInstance().getQuestStatus(id) == status;
}
void QuestHasStateCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the quest objective has status");
    ImGui::PushItemWidth(90);
    ImGui::SameLine();
    ImGui::InputInt("id", reinterpret_cast<int*>(&id), 0);
    ImGui::SameLine();
    drawEnumButton(QuestStatus::NotStarted, QuestStatus::Failed, status);
    ImGui::SameLine();
    ShowHelp("Objective ID, NOT quest ID!\nUW: Chamber 146, Restore 147, UWG 149, Vale 150, Waste 151, Pits 152, Planes 153, Mnts 154, Pools 155");
    ImGui::PopID();
}

/// ------------- KeyIsPressedCondition -------------
KeyIsPressedCondition::KeyIsPressedCondition(std::istringstream& stream)
{
    stream >> shortcutKey >> shortcutMod;
    description = makeHotkeyDescription(shortcutKey, shortcutMod);
}
void KeyIsPressedCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << shortcutKey << " " << shortcutMod << " ";
}
bool KeyIsPressedCondition::check() const
{
    bool keyIsPressed = GetAsyncKeyState(shortcutKey) & (1 << 15);
    if (shortcutMod & ModKey_Control) keyIsPressed &= ImGui::IsKeyDown(ImGuiKey_ModCtrl);
    if (shortcutMod & ModKey_Shift) keyIsPressed &= ImGui::IsKeyDown(ImGuiKey_ModShift);
    if (shortcutMod & ModKey_Alt) keyIsPressed &= ImGui::IsKeyDown(ImGuiKey_ModAlt);

    return keyIsPressed;
}
void KeyIsPressedCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If key is held down:");
    ImGui::SameLine();
    drawHotkeySelector(shortcutKey, shortcutMod, description, 100.f);
    ImGui::SameLine();
    ShowHelp("Different from the hotkey trigger, this does not block the input to Guild Wars and continuously checks if the key is pressed; not just once when it is pressed");
    ImGui::PopID();
}

/// ------------- InstanceTimeCondition -------------

InstanceTimeCondition::InstanceTimeCondition(std::istringstream& stream)
{
    stream >> timeInSeconds;
}
void InstanceTimeCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << timeInSeconds << " ";
}
bool InstanceTimeCondition::check() const
{
    return (int)(GW::Map::GetInstanceTime() / 1000) >= timeInSeconds;
}
void InstanceTimeCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the instance is older than");
    ImGui::SameLine();
    ImGui::PushItemWidth(90);
    ImGui::InputInt("s", &timeInSeconds, 0);
    ImGui::PopID();
}

/// ------------- NearbyAgentCondition -------------

NearbyAgentCondition::NearbyAgentCondition(std::istringstream& stream)
{
    stream >> agentType >> primary >> secondary >> status >> hexed >> skill >> modelId >> minDistance >> maxDistance;
    agentName = readStringWithSpaces(stream);
    polygon = readPositions(stream);
}
void NearbyAgentCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << agentType << " " << primary << " " << secondary << " " << status << " " << hexed << " " << skill << " "
           << " " << modelId << " " << minDistance << " " << maxDistance << " ";
    writeStringWithSpaces(stream, agentName);
    writePositions(stream, polygon);
}
bool NearbyAgentCondition::check() const
{
    const auto player = GW::Agents::GetPlayerAsAgentLiving();
    const auto agents = GW::Agents::GetAgentArray();
    if (!player || !agents) return false;

    auto& instanceInfo = InstanceInfo::getInstance();

    const auto fulfillsConditions = [&](const GW::AgentLiving* agent) {
        if (!agent) return false;
        const auto correctType = [&]() -> bool {
            switch (agentType) {
                case AgentType::Any:
                    return true;
                case AgentType::PartyMember: // optimize this? Dont need to check all agents
                    return agent->IsPlayer();
                case AgentType::Friendly:
                    return agent->allegiance != GW::Constants::Allegiance::Enemy;
                case AgentType::Hostile:
                    return agent->allegiance == GW::Constants::Allegiance::Enemy;
                default:
                    return false;
            }
        }();
        const auto correctPrimary = (primary == Class::Any) || primary == (Class)agent->primary;
        const auto correctSecondary = (secondary == Class::Any) || secondary == (Class)agent->secondary;
        const auto correctStatus = (status == Status::Any) || ((status == Status::Alive) == agent->GetIsAlive());
        const auto hexedCorrectly = (hexed == HexedStatus::Any) || ((hexed == HexedStatus::Hexed) == agent->GetIsHexed());
        const auto correctSkill = (skill == GW::Constants::SkillID::No_Skill) || (skill == (GW::Constants::SkillID)agent->skill);
        const auto correctModelId = (modelId == 0) || (agent->player_number == modelId);
        const auto distance = GW::GetDistance(player->pos, agent->pos);
        const auto goodDistance = (minDistance < distance) && (distance < maxDistance);
        const auto goodName = (agentName.empty()) || (instanceInfo.getDecodedName(agent->agent_id) == agentName);
        const auto goodPosition = (polygon.size() < 3u) || pointIsInsidePolygon(agent->pos, polygon);
        return correctType && correctPrimary && correctSecondary && correctStatus && hexedCorrectly && correctSkill && correctModelId && goodDistance && goodName && goodPosition;
    };
    if (agentType == AgentType::PartyMember) {
        const auto info = GW::PartyMgr::GetPartyInfo();
        if (!info) return false;
        for (const auto& partyMember : info->players) {
            const auto agent = GW::Agents::GetAgentByID(GW::Agents::GetAgentIdByLoginNumber(partyMember.login_number));
            if (!agent) continue;
            if (fulfillsConditions(agent->GetAsAgentLiving())) return true;
        }
    }
    else {
        for (const auto* agent : *agents) {
            if (!agent) continue;
            if (fulfillsConditions(agent->GetAsAgentLiving())) return true;
        }
    }
    return false;
}
void NearbyAgentCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::PushItemWidth(120);

    if (ImGui::TreeNodeEx("If there exists an agent with characteristics", ImGuiTreeNodeFlags_FramePadding)) {
        ImGui::BulletText("Distance to player");
        ImGui::SameLine();
        ImGui::InputFloat("min", &minDistance);
        ImGui::SameLine();
        ImGui::InputFloat("max", &maxDistance);

        ImGui::BulletText("Allegiance");
        ImGui::SameLine();
        drawEnumButton(AgentType::Any, AgentType::Hostile, agentType, 0);

        ImGui::BulletText("Class");
        ImGui::SameLine();
        drawEnumButton(Class::Any, Class::Dervish, primary, 1);
        ImGui::SameLine();
        ImGui::Text("/");
        ImGui::SameLine();
        drawEnumButton(Class::Any, Class::Dervish, secondary, 2);

        ImGui::BulletText("Dead or alive");
        ImGui::SameLine();
        drawEnumButton(Status::Any, Status::Alive, status, 3);

        ImGui::BulletText("Hexed");
        ImGui::SameLine();
        drawEnumButton(HexedStatus::Any, HexedStatus::Hexed, hexed, 4);

        ImGui::BulletText("Uses skill");
        ImGui::SameLine();
        ImGui::InputInt("id (0 for any)###0", reinterpret_cast<int*>(&skill), 0);

        ImGui::BulletText("Has name");
        ImGui::SameLine();
        ImGui::InputText("name (empty for any)", &agentName);

        ImGui::Bullet();
        ImGui::Text("Has model");
        ImGui::SameLine();
        ImGui::InputInt("id (0 for any)###1", &modelId, 0);

        ImGui::Bullet();
        ImGui::Text("Is within polygon");
        drawPolygonSelector(polygon);

        ImGui::TreePop();
    }
    ImGui::PopID();
}

/// ------------- CanPopAgentCondition -------------

bool CanPopAgentCondition::check() const
{
    return InstanceInfo::getInstance().canPopAgent();
}
void CanPopAgentCondition::drawSettings()
{
    ImGui::Text("If player can pop minipet or ghost in the box");
}

/// ------------- PlayerIsIdleCondition -------------

bool PlayerIsIdleCondition::check() const
{
    const auto player = GW::Agents::GetPlayerAsAgentLiving();
    if (!player) return false;
    return player->GetIsIdle();
}
void PlayerIsIdleCondition::drawSettings()
{
    ImGui::Text("If player is idle");
}

/// ------------- PlayerHasItemEquippedCondition -------------

PlayerHasItemEquippedCondition::PlayerHasItemEquippedCondition(std::istringstream& stream)
{
    stream >> modelId;
}
void PlayerHasItemEquippedCondition::serialize(std::ostringstream& stream) const
{
    Condition::serialize(stream);

    stream << modelId << " ";
}
bool PlayerHasItemEquippedCondition::check() const
{
    return FindMatchingItem(GW::Constants::Bag::Equipped_Items, modelId) != nullptr;
}
void PlayerHasItemEquippedCondition::drawSettings()
{
    ImGui::PushID(drawId());
    ImGui::Text("If the player has item equipped");
    ImGui::SameLine();
    ImGui::PushItemWidth(90);
    ImGui::InputInt("model id", &modelId, 0);
    ImGui::PopID();
}
