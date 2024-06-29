#include <enumUtils.h>

#include <commonIncludes.h>
#include <GWCA/Constants/Constants.h>

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
