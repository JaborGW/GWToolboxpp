#include "DialogsWindow.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>

#include <GWCA/Managers/AgentMgr.h>

namespace {
    static const char* const questnames[] = {
        "UW - Chamber",
        "UW - Wastes",
        "UW - UWG",
        "UW - Mnt",
        "UW - Pits",
        "UW - Planes",
        "UW - Pools",
        "UW - Escort",
        "UW - Restore",
        "UW - Vale",
        "FoW - Defend",
        "FoW - Army Of Darkness",
        "FoW - WailingLord",
        "FoW - Griffons",
        "FoW - Slaves",
        "FoW - Restore",
        "FoW - Hunt",
        "FoW - Forgemaster",
        "FoW - Tos",
        "FoW - Toc",
        "FoW - Khobay",
        "DoA - Gloom 1: Deathbringer Company",
        "DoA - Gloom 2: The Rifts Between Us",
        "DoA - Gloom 3: To The Rescue",
        "DoA - City",
        "DoA - Veil 1: Breaching Stygian Veil",
        "DoA - Veil 2: Brood Wars",
        "DoA - Foundry 1: Foundry Of Failed Creations",
        "DoA - Foundry 2: Foundry Breakout"
    };
    static const char* const dialognames[] = {
        "Craft fow armor",       "Prof Change - Warrior", "Prof Change - Ranger",   "Prof Change - Monk",        "Prof Change - Necro",       "Prof Change - Mesmer",  "Prof Change - Elementalist", "Prof Change - Assassin",    "Prof Change - Ritualist",
        "Prof Change - Paragon", "Prof Change - Dervish", "Kama -> Docks @ Hahnna", "Docks -> Kaineng @ Mhenlo", "Docks -> LA Gate @ Mhenlo", "LA Gate -> LA @ Neiro", "Faction mission outpost",    "Nightfall mission outpost",
    };
    void SetNextWindowCenter(const ImGuiWindowFlags flags)
    {
        const auto& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), flags, ImVec2(0.5f, 0.5f));
    }
    bool ParseInt(const char* str, int* val, const int base = 0)
    {
        char* end;
        *val = strtol(str, &end, base);
        if (*end != 0 || errno == ERANGE) {
            return false;
        }

        return true;
    }
} // namespace

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static DialogsWindow instance;
    return &instance;
}

void DialogsWindow::Draw(IDirect3DDevice9* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
    if (!GetVisiblePtr() || !*GetVisiblePtr()) return;
    auto DialogButton = [](int x_idx, int x_qty, const char* text, const char* help, DWORD dialog) -> void {
        if (x_idx != 0) ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        const auto windowContentRegionWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
        float w = (windowContentRegionWidth - ImGui::GetStyle().ItemInnerSpacing.x * (x_qty - 1)) / x_qty;
        if (ImGui::Button(text, ImVec2(w, 0))) {
            GW::Agents::SendDialog(dialog);
        }
        if (text != nullptr && ImGui::IsItemHovered()) {
            ImGui::SetTooltip(help);
        }
    };

    SetNextWindowCenter(ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(), GetVisiblePtr(), GetWinFlags())) {
        if (show_common) {
            DialogButton(0, 2, "Four Horseman", "Take quest in Planes", QuestAcceptDialog(GW::Constants::QuestID::UW_Planes));
            DialogButton(1, 2, "Demon Assassin", "Take quest in Mountains", QuestAcceptDialog(GW::Constants::QuestID::UW_Mnt));
            DialogButton(0, 2, "Tower of Strength", "Take quest", QuestAcceptDialog(GW::Constants::QuestID::Fow_Tos));
            DialogButton(1, 2, "Foundry Reward", "Accept reward", QuestRewardDialog(GW::Constants::QuestID::Doa_FoundryBreakout));
            DialogButton(0, 1, "Dhuum", "Take quest", 8677633);
            ImGui::Separator();
        }
        if (show_uwteles) {
            DialogButton(0, 4, "Lab", "Teleport Lab", GW::Constants::DialogID::UwTeleLab);
            DialogButton(1, 4, "Vale", "Teleport Vale", GW::Constants::DialogID::UwTeleVale);
            DialogButton(2, 4, "Pits", "Teleport Pits", GW::Constants::DialogID::UwTelePits);
            DialogButton(3, 4, "Pools", "Teleport Pools", GW::Constants::DialogID::UwTelePools);

            DialogButton(0, 3, "Planes", "Teleport Planes", GW::Constants::DialogID::UwTelePlanes);
            DialogButton(1, 3, "Wastes", "Teleport Wastes", GW::Constants::DialogID::UwTeleWastes);
            DialogButton(2, 3, "Mountains", "Teleport Mountains\nThis is NOT the mountains quest", GW::Constants::DialogID::UwTeleMnt);
            ImGui::Separator();
        }
        const size_t n_quests = _countof(questnames);
        if (show_favorites) {
            for (int i = 0; i < fav_count; ++i) {
                size_t index = static_cast<size_t>(i);
                ImGui::PushID(i);
                ImGui::PushItemWidth(-100.0f - ImGui::GetStyle().ItemInnerSpacing.x * 2);
                ImGui::Combo("", &fav_index[index], questnames, n_quests);
                ImGui::PopItemWidth();
                ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
                if (ImGui::Button("Take", ImVec2(40.0f, 0))) {
                    GW::Agents::SendDialog(QuestAcceptDialog(IndexToQuestID(fav_index[index])));
                }
                ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
                if (ImGui::Button("Reward", ImVec2(60.0f, 0))) {
                    GW::Agents::SendDialog(QuestRewardDialog(static_cast<GW::Constants::QuestID>(IndexToDialogID(fav_index[index]))));
                }
                ImGui::PopID();
            }
            ImGui::Separator();
        }
        if (show_custom) {
            const int n_dialogs = _countof(dialognames);
            static int dialogindex = 0;
            ImGui::PushItemWidth(-60.0f - ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::Combo("###dialogcombo", &dialogindex, dialognames, n_dialogs);
            ImGui::PopItemWidth();
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            if (ImGui::Button("Send##1", ImVec2(60.0f, 0))) {
                GW::Agents::SendDialog(IndexToDialogID(dialogindex));
            }

            ImGui::PushItemWidth(-60.0f - ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::InputText("###dialoginput", customdialogbuf, 64, ImGuiInputTextFlags_None);
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("You can prefix the number by \"0x\" to specify an hexadecimal number");
            }
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            if (ImGui::Button("Send##2", ImVec2(60.0f, 0))) {
                int iid;
                if (ParseInt(customdialogbuf, &iid) && (0 <= iid)) {
                    uint32_t id = static_cast<uint32_t>(iid);
                    GW::Agents::SendDialog(id);
                }
                else {
                    //Log::Error("Invalid dialog number '%s'", customdialogbuf);
                }
            }
        }
    }
    ImGui::End();
}

void DialogsWindow::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings();
    ImGui::PushItemWidth(100.0f);
    if (ImGui::InputInt("Number of favorites", &fav_count)) {
        if (fav_count < 0) fav_count = 0;
        if (fav_count > 100) fav_count = 100;
        size_t count = static_cast<size_t>(fav_count);
        fav_index.resize(count, 0);
    }
    ImGui::PopItemWidth();
    ImGui::Text("Show:");
    ImGui::Checkbox("Common 4", &show_common);
    ImGui::SameLine();
    ImGui::Checkbox("UW Teles", &show_uwteles);
    ImGui::SameLine();
    ImGui::Checkbox("Favorites", &show_favorites);
    ImGui::SameLine();
    ImGui::Checkbox("Custom", &show_custom);
}

void DialogsWindow::LoadSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    ini.LoadFile(GetSettingFile(folder).c_str());

    show_menubutton = ini.GetBoolValue(Name(), VAR_NAME(show_menubutton), true);
    fav_count = ini.GetLongValue(Name(), VAR_NAME(fav_count), 3);
    size_t count = static_cast<size_t>(fav_count);
    fav_index.resize(count, 0);
    for (size_t i = 0; i < count; ++i) {
        char key[32];
        snprintf(key, 32, "Quest%zu", i);
        fav_index[i] = ini.GetLongValue(Name(), key, 0);
    }
    show_common = ini.GetBoolValue(Name(), VAR_NAME(show_common), true);
    show_uwteles = ini.GetBoolValue(Name(), VAR_NAME(show_uwteles), true);
    show_favorites = ini.GetBoolValue(Name(), VAR_NAME(show_favorites), true);
    show_custom = ini.GetBoolValue(Name(), VAR_NAME(show_custom), true);
}

void DialogsWindow::SaveSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    ini.SetLongValue(Name(), "fav_count", fav_count);
    size_t count = static_cast<size_t>(fav_count);
    for (size_t i = 0; i < count; ++i) {
        char key[32];
        snprintf(key, 32, "Quest%zu", i);
        ini.SetLongValue(Name(), key, fav_index[i]);
    }
    ini.SetBoolValue(Name(), VAR_NAME(show_common), show_common);
    ini.SetBoolValue(Name(), VAR_NAME(show_uwteles), show_uwteles);
    ini.SetBoolValue(Name(), VAR_NAME(show_favorites), show_favorites);
    ini.SetBoolValue(Name(), VAR_NAME(show_custom), show_custom);
    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}

GW::Constants::QuestID DialogsWindow::IndexToQuestID(int index)
{
    switch (index) {
        case 0:
            return GW::Constants::QuestID::UW_Chamber;
        case 1:
            return GW::Constants::QuestID::UW_Wastes;
        case 2:
            return GW::Constants::QuestID::UW_UWG;
        case 3:
            return GW::Constants::QuestID::UW_Mnt;
        case 4:
            return GW::Constants::QuestID::UW_Pits;
        case 5:
            return GW::Constants::QuestID::UW_Planes;
        case 6:
            return GW::Constants::QuestID::UW_Pools;
        case 7:
            return GW::Constants::QuestID::UW_Escort;
        case 8:
            return GW::Constants::QuestID::UW_Restore;
        case 9:
            return GW::Constants::QuestID::UW_Vale;
        case 10:
            return GW::Constants::QuestID::Fow_Defend;
        case 11:
            return GW::Constants::QuestID::Fow_ArmyOfDarknesses;
        case 12:
            return GW::Constants::QuestID::Fow_WailingLord;
        case 13:
            return GW::Constants::QuestID::Fow_Griffons;
        case 14:
            return GW::Constants::QuestID::Fow_Slaves;
        case 15:
            return GW::Constants::QuestID::Fow_Restore;
        case 16:
            return GW::Constants::QuestID::Fow_Hunt;
        case 17:
            return GW::Constants::QuestID::Fow_Forgemaster;
        case 18:
            return GW::Constants::QuestID::Fow_Tos;
        case 19:
            return GW::Constants::QuestID::Fow_Toc;
        case 20:
            return GW::Constants::QuestID::Fow_Khobay;
        case 21:
            return GW::Constants::QuestID::Doa_DeathbringerCompany;
        case 22:
            return GW::Constants::QuestID::Doa_RiftBetweenUs;
        case 23:
            return GW::Constants::QuestID::Doa_ToTheRescue;
        case 24:
            return GW::Constants::QuestID::Doa_City;
        case 25:
            return GW::Constants::QuestID::Doa_BreachingStygianVeil;
        case 26:
            return GW::Constants::QuestID::Doa_BroodWars;
        case 27:
            return GW::Constants::QuestID::Doa_FoundryOfFailedCreations;
        case 28:
            return GW::Constants::QuestID::Doa_FoundryBreakout;
        default:
            assert(false);
            return GW::Constants::QuestID::None;
    }
}

int DialogsWindow::IndexToDialogID(int index)
{
    switch (index) {
        case 0:
            return GW::Constants::DialogID::FowCraftArmor;
        case 1:
            return GW::Constants::DialogID::ProfChangeWarrior;
        case 2:
            return GW::Constants::DialogID::ProfChangeRanger;
        case 3:
            return GW::Constants::DialogID::ProfChangeMonk;
        case 4:
            return GW::Constants::DialogID::ProfChangeNecro;
        case 5:
            return GW::Constants::DialogID::ProfChangeMesmer;
        case 6:
            return GW::Constants::DialogID::ProfChangeEle;
        case 7:
            return GW::Constants::DialogID::ProfChangeAssassin;
        case 8:
            return GW::Constants::DialogID::ProfChangeRitualist;
        case 9:
            return GW::Constants::DialogID::ProfChangeParagon;
        case 10:
            return GW::Constants::DialogID::ProfChangeDervish;
        case 11:
            return GW::Constants::DialogID::FerryKamadanToDocks;
        case 12:
            return GW::Constants::DialogID::FerryDocksToKaineng;
        case 13:
            return GW::Constants::DialogID::FerryDocksToLA;
        case 14:
            return GW::Constants::DialogID::FerryGateToLA;
        case 15:
            return GW::Constants::DialogID::FactionMissionOutpost;
        case 16:
            return GW::Constants::DialogID::NightfallMissionOutpost;
        default:
            return 0;
    }
}
