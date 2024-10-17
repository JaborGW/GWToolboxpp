#include <enumUtils.h>

#include <commonIncludes.h>
#include <Keys.h>
#include <ImGuiCppWrapper.h>

#include <GWCA/Constants/Skills.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Managers/UIMgr.h>

#include <ModelNames.h>

std::string getSkillName(GW::Constants::SkillID id, bool zeroIsAny)
{
    if (id == GW::Constants::SkillID::No_Skill) 
        return zeroIsAny ? "Any" : "No skill";
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

std::string WStringToString(const std::wstring_view str)
{
    if (str.empty()) {
        return "";
    }
    const auto size_needed = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
    std::string str_to(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), str_to.data(), size_needed, NULL, NULL);
    return str_to;
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
        case Sorting::ModelID:
            return "Model ID";
    }
    return "";
}
std::string_view toString(AnyNoYes anyNoYes) {
    switch (anyNoYes) {
        case AnyNoYes::Any:
            return "Any";
        case AnyNoYes::Yes:
            return "Yes";
        case AnyNoYes::No:
            return "No";
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
            return "/ Commands";
        case Channel::Log:
            return "Log";
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

std::string_view toString(PlayerConnectednessRequirement req) 
{
    switch (req) {
        case PlayerConnectednessRequirement::All:
            return "Everyone";
        case PlayerConnectednessRequirement::Individual:
            return "Single player";
    }
    return "";
}

std::string_view toString(Status status)
{
    switch (status) {
        case Status::Enchanted:
            return "Enchanted";
        case Status::WeaponSpelled:
            return "Weapon spelled";
        case Status::Alive:
            return "Alive";
        case Status::Bleeding:
            return "Bleeding";
        case Status::Crippled:
            return "Crippled";
        case Status::DeepWounded:
            return "Deep wounded";
        case Status::Poisoned:
            return "Poisoned";
        case Status::Hexed:
            return "Hexed";
        case Status::Idle:
            return "Idle";
        case Status::KnockedDown:
            return "Knocked down";
        case Status::Moving:
            return "Moving";
        case Status::Attacking:
            return "Attacking";
        case Status::Casting:
            return "Casting";
    }
    return "";
}

std::string_view toString(EquippedItemSlot slot)
{
    switch (slot) {
        case EquippedItemSlot::Mainhand:
            return "Main hand";
        case EquippedItemSlot::Offhand:
            return "Off hand";
        case EquippedItemSlot::Head:
            return "Head";
        case EquippedItemSlot::Chest:
            return "Chest";
        case EquippedItemSlot::Hands:
            return "Hands";
        case EquippedItemSlot::Legs:
            return "Legs";
        case EquippedItemSlot::Feet:
            return "Feet";
    }
    return "";
}

std::string_view toString(TrueFalse val)
{
    switch (val) {
        case TrueFalse::True:
            return "True";
        case TrueFalse::False:
            return "False";
    }
    return "";
}

std::string_view toString(MoveToBehaviour behaviour)
{
    switch (behaviour) {
        case MoveToBehaviour::SendOnce:
            return "Finish when at target or not moving";
        case MoveToBehaviour::RepeatIfIdle:
            return "Finish when at target, repeat move if not moving";
        case MoveToBehaviour::ImmediateFinish:
            return "Immediately finish";
    }
    return "";
}

std::string_view toString(ReferenceFrame refFrame)
{
    switch (refFrame) {
        case ReferenceFrame::Player:
            return "Player viewing direction";
        case ReferenceFrame::Camera:
            return "Camera viewing direction";
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
        case Trigger::ChatMessage:
            return "Trigger on chat message";
        case Trigger::BeginSkillCast:
            return "Trigger on begin skill cast";
        case Trigger::SkillCastInterrupt:
            return "Trigger on interrupt";
        case Trigger::BeginCooldown:
            return "Trigger on end skill cast";
    }
    return "";
}

std::string_view toString(GW::Constants::InstanceType type) 
{
    switch (type) {
        case GW::Constants::InstanceType::Explorable:
            return "Explorable";
        case GW::Constants::InstanceType::Loading:
            return "Loading";
        case GW::Constants::InstanceType::Outpost:
            return "Outpost";
    }
    return "";
}
std::string_view toString(WeaponType type)
{
    switch (type) {
        case WeaponType::Any:
            return "Any";
        case WeaponType::None:
            return "None";
        case WeaponType::Bow:
            return "Bow";
        case WeaponType::Axe:
            return "Axe";
        case WeaponType::Hammer:
            return "Hammer";
        case WeaponType::Daggers:
            return "Daggers";
        case WeaponType::Scythe:
            return "Scythe";
        case WeaponType::Spear:
            return "Spear";
        case WeaponType::Sword:
            return "Sword";
        case WeaponType::Wand:
            return "Wand";
        case WeaponType::Staff:
            return "Staff";
    }
    return "";
}
std::string_view toString(Bag bag) 
{
    switch (bag) {
        case Bag::Backpack:
            return "Backpack";
        case Bag::BeltPouch:
            return "Belt Pouch";
        case Bag::Bag1:
            return "Bag 1";
        case Bag::Bag2:
            return "Bag 2";
        case Bag::EquipmentPack:
            return "Equipment Pack";
    }
    return "";
}

std::string_view toString(ComparisonOperator comp)
{
    switch (comp) 
    {
        case ComparisonOperator::Equals:
            return "==";
        case ComparisonOperator::Less:
            return "<";
        case ComparisonOperator::Greater:
            return ">";
        case ComparisonOperator::LessOrEqual:
            return "<=";
        case ComparisonOperator::GreaterOrEqual:
            return ">=";
        case ComparisonOperator::NotEquals:
            return "!=";
    }
    return "";
}

std::string_view toString(IsIsNot comp)
{
    switch (comp) 
    {
        case IsIsNot::Is:
            return "is";
        case IsIsNot::IsNot:
            return "is not";
    }
    return "";
}

std::string_view toString(GW::Constants::HeroID hero) 
{
    switch (hero)
    {
        using GW::Constants::HeroID;
        case HeroID::NoHero:
            return "Any";
        case HeroID::Norgu:
            return "Norgu";
        case HeroID::Goren:
            return "Goren";
        case HeroID::Tahlkora:
            return "Tahlkora";
        case HeroID::MasterOfWhispers:
            return "Master of Whispers";
        case HeroID::AcolyteJin:
            return "Acolyte Jin";
        case HeroID::Koss:
            return "Koss";
        case HeroID::Dunkoro:
            return "Dunkoro";
        case HeroID::AcolyteSousuke:
            return "Acolyte Sousuke";
        case HeroID::Melonni:
            return "Melonni";
        case HeroID::ZhedShadowhoof:
            return "Zhed Shadowhoof";
        case HeroID::GeneralMorgahn:
            return "General Morgahn";
        case HeroID::MargridTheSly:
            return "Margrid the Sly";
        case HeroID::Zenmai:
            return "Zenmai";
        case HeroID::Olias:
            return "Olias";
        case HeroID::Razah:
            return "Razah";
        case HeroID::MOX:
            return "M.O.X.";
        case HeroID::KeiranThackeray:
            return "Keiran Thackeray";
        case HeroID::Jora:
            return "Jora";
        case HeroID::PyreFierceshot:
            return "Pyre Fierceshot";
        case HeroID::Anton:
            return "Anton";
        case HeroID::Livia:
            return "Livia";
        case HeroID::Hayda:
            return "Hayda";
        case HeroID::Kahmu:
            return "Kahmu";
        case HeroID::Gwen:
            return "Gwen";
        case HeroID::Xandra:
            return "Xandra";
        case HeroID::Vekk:
            return "Vekk";
        case HeroID::Ogden:
            return "Ogden";
        case HeroID::Merc1:
            return "Mercenary 1";
        case HeroID::Merc2:
            return "Mercenary 2";
        case HeroID::Merc3:
            return "Mercenary 3";
        case HeroID::Merc4:
            return "Mercenary 4";
        case HeroID::Merc5:
            return "Mercenary 5";
        case HeroID::Merc6:
            return "Mercenary 6";
        case HeroID::Merc7:
            return "Mercenary 7";
        case HeroID::Merc8:
            return "Mercenary 8";
        case HeroID::Miku:
            return "Miku";
        case HeroID::ZeiRi:
            return "Zei Ri";
    }
    return "";
} 

std::string_view toString(GW::UI::ControlAction action) 
{
    constexpr auto names = std::array{
        "Action: Attack/Interact (Do It)",
        "Inventory: Activate Weapon Set 1",
        "Inventory: Activate Weapon Set 2",
        "Inventory: Activate Weapon Set 3",
        "Inventory: Activate Weapon Set 4",
        "Panel: Close All Panels",
        "Inventory: Cycle Equipment",
        "Miscellaneous: Log Out",
        "Chat: Open Chat",
        "Targeting: Show Others",
        "Panel: Open Hero",
        "Inventory: Open Inventory",
        "Panel: Open World Map",
        "Panel: Open Options",
        "Panel: Open Quest",
        "Panel: Open Skills and Attributes",
        "Camera: Reverse Camera",
        "Movement: Strafe Left",
        "Movement: Strafe Right",
        "Targeting: Foe - Nearest",
        "Targeting: Show Targets",
        "Targeting: Foe - Next",
        "Targeting: Party Member - 1",
        "Targeting: Party Member - 2",
        "Targeting: Party Member - 3",
        "Targeting: Party Member - 4",
        "Targeting: Party Member - 5",
        "Targeting: Party Member - 6",
        "Targeting: Party Member - 7",
        "Targeting: Party Member - 8",
        "Targeting: Foe - Previous",
        "Targeting: Priority Target",
        "Targeting: Self",
        "Chat: Toggle Chat",
        "Movement: Turn Left",
        "Movement: Turn Right",
        "Action: Use Skill 1",
        "Action: Use Skill 2",
        "Action: Use Skill 3",
        "Action: Use Skill 4",
        "Action: Use Skill 5",
        "Action: Use Skill 6",
        "Action: Use Skill 7",
        "Action: Use Skill 8",
        "Movement: Move Backward",
        "Movement: Move Forward",
        "Miscellaneous: Screenshot",
        "Action: Cancel Action",
        "Camera: Free Camera",
        "Movement: Reverse Direction",
        "Inventory: Open Backpack",
        "Inventory: Open Belt Pouch",
        "Inventory: Open Bag 1",
        "Inventory: Open Bag 2",
        "Panel: Open Mission Map",
        "Movement: Automatic Run",
        "Inventory: Toggle All Bags",
        "Panel: Open Friends",
        "Panel: Open Guild",
        "Miscellaneous: Language Quick Toggle",
        "Targeting: Ally - Nearest",
        "Panel: Open Score Chart",
        "Chat: Reply",
        "Panel: Open Party",
        "Chat: Start Chat Command",
        "Panel: Open Customize Layout",
        "Panel: Open Observe",
        "Targeting: Item - Nearest",
        "Targeting: Item - Next",
        "Targeting: Item - Previous",
        "Targeting: Party Member - 9",
        "Targeting: Party Member - 10",
        "Targeting: Party Member - 11",
        "Targeting: Party Member - 12",
        "Targeting: Party Member - Next",
        "Targeting: Party Member - Previous",
        "Action: Follow",
        "Action: Drop Item",
        "Camera: Zoom In",
        "Camera: Zoom Out",
        "Action: Suppress Action",
        "Panel: Open Load from Equipment Template",
        "Panel: Open Load from Skills Template",
        "Panel: Open Templates Manager",
        "Panel: Open Save to Equipment Template",
        "Panel: Open Save to Skills Template",
        "Action: Command Party",
        "Action: Command Hero 1",
        "Action: Command Hero 2",
        "Action: Command Hero 3",
        "Panel: Open PvP Equipment",
        "Action: Clear Party Commands",
        "Panel: Open Hero Commander 1",
        "Panel: Open Hero Commander 2",
        "Panel: Open Hero Commander 3",
        "Panel: Open Pet Commander",
        "Panel: Open Hero 1 Pet Commander",
        "Panel: Open Hero 2 Pet Commander",
        "Panel: Open Hero 3 Pet Commander",
        "Targeting: Clear Target",
        "Panel: Open Help",
        "Action: Order Hero 1 to use Skill 1",
        "Action: Order Hero 1 to use Skill 2",
        "Action: Order Hero 1 to use Skill 3",
        "Action: Order Hero 1 to use Skill 4",
        "Action: Order Hero 1 to use Skill 5",
        "Action: Order Hero 1 to use Skill 6",
        "Action: Order Hero 1 to use Skill 7",
        "Action: Order Hero 1 to use Skill 8",
        "Action: Order Hero 2 to use Skill 1",
        "Action: Order Hero 2 to use Skill 2",
        "Action: Order Hero 2 to use Skill 3",
        "Action: Order Hero 2 to use Skill 4",
        "Action: Order Hero 2 to use Skill 5",
        "Action: Order Hero 2 to use Skill 6",
        "Action: Order Hero 2 to use Skill 7",
        "Action: Order Hero 2 to use Skill 8",
        "Action: Order Hero 3 to use Skill 1",
        "Action: Order Hero 3 to use Skill 2",
        "Action: Order Hero 3 to use Skill 3",
        "Action: Order Hero 3 to use Skill 4",
        "Action: Order Hero 3 to use Skill 5",
        "Action: Order Hero 3 to use Skill 6",
        "Action: Order Hero 3 to use Skill 7",
        "Action: Order Hero 3 to use Skill 8",
        "Panel: Open Minion List",
        "Panel: Open Hero 4 Pet Commander",
        "Panel: Open Hero 5 Pet Commander",
        "Panel: Open Hero 6 Pet Commander",
        "Panel: Open Hero 7 Pet Commander",
        "Action: Command Hero 4",
        "Action: Command Hero 5",
        "Action: Command Hero 6",
        "Action: Command Hero 7",
        "Action: Order Hero 4 to use Skill 1",
        "Action: Order Hero 4 to use Skill 2",
        "Action: Order Hero 4 to use Skill 3",
        "Action: Order Hero 4 to use Skill 4",
        "Action: Order Hero 4 to use Skill 5",
        "Action: Order Hero 4 to use Skill 6",
        "Action: Order Hero 4 to use Skill 7",
        "Action: Order Hero 4 to use Skill 8",
        "Action: Order Hero 5 to use Skill 1",
        "Action: Order Hero 5 to use Skill 2",
        "Action: Order Hero 5 to use Skill 3",
        "Action: Order Hero 5 to use Skill 4",
        "Action: Order Hero 5 to use Skill 5",
        "Action: Order Hero 5 to use Skill 6",
        "Action: Order Hero 5 to use Skill 7",
        "Action: Order Hero 5 to use Skill 8",
        "Action: Order Hero 6 to use Skill 1",
        "Action: Order Hero 6 to use Skill 2",
        "Action: Order Hero 6 to use Skill 3",
        "Action: Order Hero 6 to use Skill 4",
        "Action: Order Hero 6 to use Skill 5",
        "Action: Order Hero 6 to use Skill 6",
        "Action: Order Hero 6 to use Skill 7",
        "Action: Order Hero 6 to use Skill 8",
        "Action: Order Hero 7 to use Skill 1",
        "Action: Order Hero 7 to use Skill 2",
        "Action: Order Hero 7 to use Skill 3",
        "Action: Order Hero 7 to use Skill 4",
        "Action: Order Hero 7 to use Skill 5",
        "Action: Order Hero 7 to use Skill 6",
        "Action: Order Hero 7 to use Skill 7",
        "Action: Order Hero 7 to use Skill 8",
        "Panel: Open Hero Commander 4",
        "Panel: Open Hero Commander 5",
        "Panel: Open Hero Commander 6",
        "Panel: Open Hero Commander 7"
    };
    return names[static_cast<int>(action - 0x80)];
}

bool checkWeaponType(WeaponType targetType, uint16_t gameType) 
{
    switch (targetType) {
        case WeaponType::Any:
            return true;
        case WeaponType::None:
            return gameType == 0;
        case WeaponType::Bow:
            return gameType == 1;
        case WeaponType::Axe:
            return gameType == 2;
        case WeaponType::Hammer:
            return gameType == 3;
        case WeaponType::Daggers:
            return gameType == 4;
        case WeaponType::Scythe:
            return gameType == 5;
        case WeaponType::Spear:
            return gameType == 6;
        case WeaponType::Sword:
            return gameType == 7;
        case WeaponType::Wand:
            return gameType == 10;
        case WeaponType::Staff:
            return gameType == 9 || gameType == 12 || gameType == 14;
    }
    return false;
}

std::string makeHotkeyDescription(Hotkey hotkey) 
{
    char newDescription[256];
    ModKeyName(newDescription, _countof(newDescription), hotkey.modifier, hotkey.keyData);
    return std::string{newDescription};
}

void drawHotkeySelector(Hotkey& hotkey, std::string& description, float selectorWidth) 
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
            hotkey.keyData = newkey;
            hotkey.modifier = newmod;
            description = makeHotkeyDescription(hotkey);
            newkey = 0;
            ImGui::CloseCurrentPopup();
        }

        ModKeyName(newkey_buf, _countof(newkey_buf), newmod, newkey);
        ImGui::Text("%s", newkey_buf);

        ImGui::EndPopup();
    }
    ImGui::PopItemWidth();
}

void drawTriggerSelector(Trigger& trigger, TriggerData& triggerData, float width)
{
    if (trigger == Trigger::None) 
    {
        drawEnumButton(Trigger::InstanceLoad, Trigger::SkillCastInterrupt, trigger, 0, 100.f, "Add trigger");
    }
    else if (trigger == Trigger::Hotkey) 
    {
        auto description = triggerData.hotkey.keyData ? makeHotkeyDescription(triggerData.hotkey) : "Click to change key";
        drawHotkeySelector(triggerData.hotkey, description, width - 20.f);
        ImGui::SameLine();
        ImGui::Text("Hotkey");
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20.f, 0))) {
            trigger = Trigger::None;
            triggerData.hotkey.keyData = 0;
            triggerData.hotkey.modifier = 0;
        }
    }
    else if (trigger == Trigger::ChatMessage)
    {
        ImGui::PushItemWidth(200);
        ImGui::InputText("Trigger message", &triggerData.message);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20.f, 0))) {
            trigger = Trigger::None;
            triggerData.message = "";
        }
    }
    else if (trigger == Trigger::BeginSkillCast) 
    {
        ImGui::Text("Trigger on begin skill");
        ImGui::SameLine();
        drawSkillIDSelector(triggerData.skillId, true);
        ImGui::SameLine();
        drawEnumButton(AnyNoYes::Any, AnyNoYes::Yes, triggerData.hsr, 0, 50.f);
        ImGui::SameLine();
        ImGui::Text("HSR");
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20.f, 0))) {
            trigger = Trigger::None;
            triggerData.skillId = GW::Constants::SkillID::No_Skill;
            triggerData.hsr = AnyNoYes::Any;
        }
    }
    else if (trigger == Trigger::SkillCastInterrupt) 
    {
        ImGui::Text("Trigger on interrupt of");
        ImGui::SameLine();
        drawSkillIDSelector(triggerData.skillId, true);
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20.f, 0))) {
            trigger = Trigger::None;
            triggerData.skillId = GW::Constants::SkillID::No_Skill;
        }
    }
    else if (trigger == Trigger::BeginCooldown) 
    {
        ImGui::Text("Trigger on end skill");
        ImGui::SameLine();
        drawSkillIDSelector(triggerData.skillId, true);
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20.f, 0))) {
            trigger = Trigger::None;
            triggerData.skillId = GW::Constants::SkillID::No_Skill;
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
        if (const auto player = GW::Agents::GetControlledCharacter()) {
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
    ImGui::PopItemWidth();
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

void drawSkillIDSelector(GW::Constants::SkillID& id, bool zeroIsAny)
{
    ImGui::PushItemWidth(50.f);
    ImGui::Text("%s", getSkillName(id, zeroIsAny).c_str());
    ImGui::SameLine();
    ImGui::SameLine();
    ImGui::InputInt("Skill ID", reinterpret_cast<int*>(&id), 0);
    ImGui::PopItemWidth();
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
    ImGui::PopItemWidth();
}
void drawModelIDSelector(uint16_t& id, std::optional<std::string_view> label)
{
    ImGui::PushItemWidth(50.f);
    const auto& modelNames = getModelNames();
    const auto& modelNameIt = modelNames.find(id);
    if (modelNameIt != modelNames.end())
    {
        ImGui::Text("%s", modelNameIt->second.data());
        ImGui::SameLine();
    }
    int editValue = id;

    if (ImGui::InputInt(label ? label->data() : "Model ID", &editValue, 0))
    {
        if (editValue >= 0 && editValue <= 0xFFFF)
            id = uint16_t(editValue);
        else
            id = 0;
    }
    ImGui::PopItemWidth();
}