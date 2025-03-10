#include "SkinChanger.h"

#include <cstdint>

#include <GWCA/GWCA.h>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Packets/StoC.h>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/NPC.h>

#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/WorldContext.h>

#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/AgentMgr.h>

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Utilities/Hooker.h>

#include "PluginUtils.h"
#include "BackupManager.h"

#include <sstream>

namespace 
{
    GW::HookEntry UseItem_Entry;
    GW::HookEntry AgentAdd_Entry;
    GW::HookEntry InstanceLoadFile_Entry;

    std::map<GW::Constants::Bag, std::vector<InventoryItem>> available_items;
    std::unordered_map<std::wstring, std::wstring> decodedNames;

    static std::optional<MinipetTransmog> pendingMinipetTransmog;

    std::string removeTextInBrackets(std::string str)
    {
        while (true)
        {
            const auto left = str.find('<');
            if (left == std::string::npos) return str;
            const auto right = str.find('>', left);
            if (right == std::string::npos) return str;
            str.erase(left, right + 1);
        }
    }
    std::string decode(const std::wstring& wstring) 
    {
        if (wstring.empty()) return "";
        auto& decoded = decodedNames[wstring];
        if (decoded.empty()) 
        {
            GW::GameThread::Enqueue([wstring, &decoded] { GW::UI::AsyncDecodeStr(wstring.c_str(), &decoded); });
        }
        return removeTextInBrackets(PluginUtils::WStringToString(decoded));
    }

    std::optional<int> toInt(std::string str)
    {
        try 
        {
            const auto base = str.starts_with("0x") ? 16 : 10;
            return std::stoi(str, nullptr, base);
        }
        catch (...) 
        {
            return std::nullopt;
        }
    }

    bool IsEquippable(const GW::Item* item)
    {
        if (!item) 
        {
            return false;
        }

        switch (static_cast<GW::Constants::ItemType>(item->type)) {
            case GW::Constants::ItemType::Axe:
            case GW::Constants::ItemType::Boots:
            case GW::Constants::ItemType::Bow:
            case GW::Constants::ItemType::Chestpiece:
            case GW::Constants::ItemType::Offhand:
            case GW::Constants::ItemType::Gloves:
            case GW::Constants::ItemType::Hammer:
            case GW::Constants::ItemType::Headpiece:
            case GW::Constants::ItemType::Leggings:
            case GW::Constants::ItemType::Wand:
            case GW::Constants::ItemType::Shield:
            case GW::Constants::ItemType::Staff:
            case GW::Constants::ItemType::Sword:
            case GW::Constants::ItemType::Daggers:
            case GW::Constants::ItemType::Scythe:
            case GW::Constants::ItemType::Spear:
            case GW::Constants::ItemType::Costume_Headpiece:
            case GW::Constants::ItemType::Costume:
                return true;
            default:
                return false;
        }
    }

    enum class Behaviour { AllItems, EquippableOnly };
    void forEachItem(std::function<bool(GW::Item*)> func, Behaviour behaviour) 
    {
        using GW::Constants::Bag;
        for (auto bagSlot : {Bag::Backpack, Bag::Belt_Pouch, Bag::Bag_1, Bag::Bag_2, Bag::Equipment_Pack, Bag::Equipped_Items}) 
        {
            const auto bag = GW::Items::GetBag(bagSlot);
            if (!bag) continue;

            const auto& items = bag->items;
            for (size_t slot = 0; slot < items.size(); slot++) 
            {
                const auto& item = items[slot];
                switch (behaviour) 
                {
                case Behaviour::AllItems:
                    if (item) 
                    {
                        if (func(item)) return;
                    }
                    break;
                case Behaviour::EquippableOnly:
                    if (IsEquippable(item)) 
                    {
                        if (func(item)) return;
                    }
                    break;
                }
            }
        }
    }

    std::vector<GW::ItemModifier> extractMods(const GW::Item* item)
    {
        std::vector<GW::ItemModifier> result;
        result.resize(item->mod_struct_size);
        for (auto i = 0u; i < item->mod_struct_size; ++i) 
        {
            result[i] = item->mod_struct[i];
        }
        return result;
    }

    bool compareMods(const std::vector<GW::ItemModifier>& vec, const GW::ItemModifier* ptr, size_t size) 
    {
        if (vec.size() != size) return false;
        for (size_t i = 0; i < size; ++i)
            if (vec[i].mod != ptr[i].mod) return false;
        return true;
    }

    void drawItemSelector(InventoryItem& inventoryItem) 
    {
        static bool needToFetchBagItems = false;
        constexpr auto bags = std::array{"None", "Backpack", "Belt Pouch", "Bag 1", "Bag 2", "Equipment Pack"};

        if (ImGui::Button("Edit item")) {
            needToFetchBagItems = true;
            ImGui::OpenPopup("Choose item to adjust");
        }
        ImGui::SameLine();

        if (inventoryItem.modelID == 0) 
        {
            ImGui::TextUnformatted("No item chosen");
            ImGui::SameLine();
        }
        else if (!inventoryItem.encodedName.empty())
        {
            ImGui::TextUnformatted(decode(inventoryItem.encodedName).c_str());
            ImGui::SameLine();
        }
        else 
        {
            // Encoded names are not serialized to avoid wstring headache (and maybe issues when the game updates). Find the name.
            forEachItem([&inventoryItem](const GW::Item* item) 
            {
                if (item->model_id == inventoryItem.modelID && compareMods(inventoryItem.modifiers, item->mod_struct, item->mod_struct_size)) 
                {
                    inventoryItem.encodedName = item->single_item_name;
                    return true;
                }
                return false;
            }, Behaviour::EquippableOnly);
        }
        
        if (ImGui::BeginPopupModal("Choose item to adjust", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            if (needToFetchBagItems) 
            {
                available_items.clear();
                forEachItem([&](const GW::Item* item)
                {
                    const auto bag = item->bag ? item->bag->bag_id() : GW::Constants::Bag::Backpack;
                    available_items[bag].push_back(InventoryItem{item->model_id, item->single_item_name, extractMods(item)});
                    return false;
                }, Behaviour::EquippableOnly);
                needToFetchBagItems = false;
            }
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
            for (auto& [bag, bagItems] : available_items) 
            {
                ImGui::TextUnformatted(bag == GW::Constants::Bag::Equipped_Items ? "Equipped items" : bags[(uint32_t)bag]);
                ImGui::Indent();
                for (auto& bagItem : bagItems) 
                {
                    ImGui::PushID(&bagItem);
                    if (ImGui::Button(decode(bagItem.encodedName).c_str())) 
                    {
                        inventoryItem = bagItem;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopID();
                }
                ImGui::Unindent();
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();

            if (ImGui::Button("Cancel")) 
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    constexpr ImVec4 palette[] = {
        {0.f, 0.f, 1.f, 0.f},       // Blue
        {0.f, 0.75f, 0.f, 0.f},     // Green
        {0.5f, 0.f, 0.5f, 0.f},     // Purple
        {1.f, 0.f, 0.f, 0.f},       // Red
        {1.f, 1.f, 0.f, 0.f},       // Yellow
        {0.5f, 0.25f, 0.f, 0.f},    // Brown
        {1.f, 0.65f, 0.f, 0.f},     // Orange
        {0.75f, 0.75f, 0.75f, 0.f}, // Silver
        {0.f, 0.f, 0.f, 0.f},       // Black
        {0.5f, 0.5f, 0.5f, 0.f},    // Gray
        {1.f, 1.f, 1.f, 0.f},       // White
        {0.95f, 0.5f, 0.95f, 0.f},  // Pink
    };
    ImVec4 ImVec4FromDyeColor(GW::DyeColor color)
    {
        const uint32_t color_id = std::to_underlying(color) - std::to_underlying(GW::DyeColor::Blue);
        switch (color) {
            case GW::DyeColor::Blue:
            case GW::DyeColor::Green:
            case GW::DyeColor::Purple:
            case GW::DyeColor::Red:
            case GW::DyeColor::Yellow:
            case GW::DyeColor::Brown:
            case GW::DyeColor::Orange:
            case GW::DyeColor::Silver:
            case GW::DyeColor::Black:
            case GW::DyeColor::Gray:
            case GW::DyeColor::White:
            case GW::DyeColor::Pink:
                assert(color_id < _countof(palette));
                return palette[color_id];
            default:
                return {};
        }
    }

    GW::DyeColor DyeColorFromInt(size_t color)
    {
        const auto col = static_cast<GW::DyeColor>(color);
        switch (col) {
            case GW::DyeColor::Blue:
            case GW::DyeColor::Green:
            case GW::DyeColor::Purple:
            case GW::DyeColor::Red:
            case GW::DyeColor::Yellow:
            case GW::DyeColor::Brown:
            case GW::DyeColor::Orange:
            case GW::DyeColor::Silver:
            case GW::DyeColor::Black:
            case GW::DyeColor::Gray:
            case GW::DyeColor::White:
            case GW::DyeColor::Pink:
                return col;
            default:
                return GW::DyeColor::None;
        }
    }

    bool drawDyePicker(const char* label, GW::DyeColor* color)
    {
        ImGui::PushID(label);

        const ImVec4 current_color = ImVec4FromDyeColor(*color);

        bool value_changed = false;
        const char* label_display_end = ImGui::FindRenderedTextEnd(label);

        if (ImGui::ColorButton("##ColorButton", current_color, *color == GW::DyeColor::None ? ImGuiColorEditFlags_AlphaPreview : 0)) {
            ImGui::OpenPopup("picker");
        }

        if (ImGui::BeginPopup("picker")) {
            if (label != label_display_end) {
                ImGui::TextUnformatted(label, label_display_end);
                ImGui::Separator();
            }
            size_t palette_index;
            if (ImGui::ColorPalette("##picker", &palette_index, palette, _countof(palette), 7, ImGuiColorEditFlags_AlphaPreview)) {
                if (palette_index < _countof(palette)) {
                    *color = DyeColorFromInt(palette_index + static_cast<size_t>(GW::DyeColor::Blue));
                }
                else {
                    *color = GW::DyeColor::None;
                }
                value_changed = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
        return value_changed;
    }

    void TransmoAgent(DWORD agent_id, MinipetTransmog&& transmo)
    {
        if (!transmo.npcID || !agent_id) {
            return;
        }
        const auto agent = static_cast<GW::AgentLiving*>(GW::Agents::GetAgentByID(agent_id));
        if (!agent || !agent->GetIsLivingType()) return;
        const auto existingNpc = GW::Agents::GetNPCByID(agent->player_number);
        const auto scale = existingNpc ? existingNpc->scale : 0x64000000;

        const auto& npcs = GW::GetGameContext()->world->npcs;
        if (transmo.npcID >= npcs.size() || !npcs[transmo.npcID].model_file_id) 
        {
            const auto& flags = transmo.flags;
            if (!transmo.npcModelFileID) return;

            // Need to create the NPC.
            // Those 2 packets (P074 & P075) are used to create a new model, for instance if we want to "use" a tonic.
            // We have to find the data that are in the NPC structure and feed them to those 2 packets.
            GW::NPC npc = {0};
            npc.model_file_id = transmo.npcModelFileID;
            npc.npc_flags = flags;
            npc.primary = 1;
            npc.default_level = 0;
            GW::GameThread::Enqueue([npcID = transmo.npcID, npc] {
                GW::Packet::StoC::NpcGeneralStats packet{};
                packet.npc_id = npcID;
                packet.file_id = npc.model_file_id;
                packet.data1 = 0;
                packet.scale = npc.scale;
                packet.data2 = 0;
                packet.flags = npc.npc_flags;
                packet.profession = npc.primary;
                packet.level = npc.default_level;
                packet.name[0] = 0;
                GW::StoC::EmulatePacket(&packet);
            });

            if (transmo.npcModelFileData) 
            {
                GW::GameThread::Enqueue([npcID = transmo.npcID, npcModelFileData = transmo.npcModelFileData] {
                    GW::Packet::StoC::NPCModelFile packet;
                    packet.npc_id = npcID;
                    packet.count = 1;
                    packet.data[0] = npcModelFileData;

                    GW::StoC::EmulatePacket(&packet);
                });
            }
        }
        GW::GameThread::Enqueue([npcID = transmo.npcID, agent_id, scale] 
        {
            GW::Packet::StoC::AgentScale packet1;
            packet1.header = GW::Packet::StoC::AgentScale::STATIC_HEADER;
            packet1.agent_id = agent_id;
            packet1.scale = scale;
            GW::StoC::EmulatePacket(&packet1);

            GW::Packet::StoC::AgentModel packet2;
            packet2.header = GW::Packet::StoC::AgentModel::STATIC_HEADER;
            packet2.agent_id = agent_id;
            packet2.model_id = npcID;
            GW::StoC::EmulatePacket(&packet2);
        });
    }
} // namespace

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static SkinChanger instance;
    return &instance;
}

void SkinChanger::LoadSettings(const wchar_t* folder)
{
    ToolboxPlugin::LoadSettings(folder);
    BackupManager::getInstance().initialize(folder);

    loadFromIniFile(GetSettingFile(folder).c_str());

    if (itemChanges.empty() && BackupManager::getInstance().backupCount(PluginUtils::StringToWString(Name())) > 0)
    {
        PluginUtils::logMessage("No runs loaded, but automatic backups found. Type \"/restore " + std::string{Name()} + " help\" to see options for restoring backups", Name());
    }
}

void SkinChanger::SaveSettings(const wchar_t* folder)
{
    ToolboxPlugin::SaveSettings(folder);
    
    std::string itemsToSave;
    itemsToSave.reserve(4096);
    for (const auto& itemChange : itemChanges) 
    {
        itemsToSave += std::to_string(itemChange.item.modelID) + " ";
        for (const auto& mod : itemChange.item.modifiers) 
        {
            itemsToSave += std::to_string(mod.mod) + " ";
        }
        itemsToSave += "S ";
        itemsToSave += itemChange.modelFileID + " ";
        for (const auto& dye : itemChange.dyes)
            itemsToSave += std::to_string((int)dye) + " ";
        itemsToSave += std::to_string(itemChange.tint) + " ";
        itemsToSave += std::to_string((int)itemChange.enableDyes) + " ";
        itemsToSave += "END ";
    }

    ini.SetValue(Name(), VAR_NAME(itemChanges), itemsToSave.c_str());
    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);

    if (itemChanges.size()) BackupManager::getInstance().save(PluginUtils::StringToWString(Name()), GetSettingFile(folder));
}

void SkinChanger::DrawSettings()
{
    ToolboxPlugin::DrawSettings();

    if (GW::Map::GetInstanceType() == GW::Constants::InstanceType::Loading || !GW::Agents::GetControlledCharacter()) 
    {
        return;
    }
    {
        int index = 0;
        std::optional<int> indexToDelete;
        for (auto& itemChange : itemChanges) {
            ImGui::PushID(index++);

            if (ImGui::Button("X")) indexToDelete = index - 1;
            ImGui::SameLine();

            drawItemSelector(itemChange.item);
            ImGui::SameLine();

            ImGui::PushItemWidth(80.f);
            ImGui::InputText("", &itemChange.modelFileID);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Model file ID");
            }
            std::erase_if(itemChange.modelFileID, [](auto c) {
                return std::isspace(c);
            });
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::PushID(&itemChange.enableDyes);
            ImGui::Checkbox("", &itemChange.enableDyes);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enable dyes");
            }
            ImGui::PopID();

            if (itemChange.enableDyes) {
                ImGui::SameLine();
                drawDyePicker("Dye 1", &itemChange.dyes[0]);
                ImGui::SameLine();
                drawDyePicker("Dye 2", &itemChange.dyes[1]);
                ImGui::SameLine();
                drawDyePicker("Dye 3", &itemChange.dyes[2]);
                ImGui::SameLine();
                drawDyePicker("Dye 4", &itemChange.dyes[3]);
                ImGui::SameLine();
                int tint = itemChange.tint;
                ImGui::PushItemWidth(50.f);
                ImGui::InputInt("Tint (0-255)", &tint, 0);
                ImGui::PopItemWidth();
                if (tint < 0) tint = 0;
                if (tint > 255) tint = 255;
                itemChange.tint = (uint8_t)tint;
            }

            ImGui::PopID();
        }
        if (ImGui::Button("+")) {
            itemChanges.push_back({{}, "0x", false, {GW::DyeColor::None, GW::DyeColor::None, GW::DyeColor::None, GW::DyeColor::None}, 255});
        }
        if (indexToDelete) itemChanges.erase(itemChanges.begin() + *indexToDelete);

        ImGui::Text("Item changes are applied on instance load.");
    }

    // ----------------
    {
        ImGui::Text("Saved NPC transmogs. Write `/skinchanger save <name>` with a NPC as a target to add to this");

        int index = 0;
        std::optional<int> indexToDelete;
        for (auto& npcTransmog : npcTransmogs) {
            ImGui::PushID(index++ + itemChanges.size());

            if (ImGui::Button("X")) indexToDelete = index - 1;
            ImGui::SameLine();

            ImGui::Text(npcTransmog.identifier.c_str());
            ImGui::SameLine();

            float scalePercent = (float)((double)npcTransmog.scale / 0x64000000);
            ImGui::PushItemWidth(80.f);
            ImGui::InputFloat("Scale", &scalePercent, 0.f);
            ImGui::PopItemWidth();
            npcTransmog.scale = (DWORD)(0x64000000 * scalePercent);

            ImGui::PopID();
        }
        if (indexToDelete) npcTransmogs.erase(npcTransmogs.begin() + *indexToDelete);
    }
    // -----------

    ImGui::Text("Version 1.0. For new releases, feature requests and bug reports check out");
    ImGui::SameLine();

    constexpr auto discordInviteLink = "https://discord.gg/ZpKzer4dK9";
    ImGui::TextColored(ImColor{102, 187, 238, 255}, discordInviteLink);
    if (ImGui::IsItemClicked()) {
        ShellExecute(nullptr, "open", discordInviteLink, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void SkinChanger::loadFromIniFile(const wchar_t* filePath)
{
    ini.LoadFile(filePath);
    itemChanges.clear();

    std::string loadedItems = ini.GetValue(Name(), VAR_NAME(itemChanges), "");
    if (loadedItems.empty()) return;

    std::stringstream ss{loadedItems};

    while (ss) {
        ItemChange itemChange;

        ss >> std::ws >> itemChange.item.modelID >> std::ws;

        uint32_t readMod;
        while (ss && ss.peek() != 'S') {
            ss >> readMod;
            itemChange.item.modifiers.push_back({readMod});
            ss >> std::ws;
        }
        if (ss && ss.peek() == 'S')
            ss.get();
        else
            return;

        ss >> std::ws;
        if (ss) ss >> itemChange.modelFileID;

        int readDye;
        for (auto& dye : itemChange.dyes) {
            ss >> readDye;
            dye = (GW::DyeColor)(readDye);
        }
        {
            // String stream for uint8_t does not what you would expect
            int read;
            ss >> read;
            if (read >= 0 && read < 256) itemChange.tint = (uint8_t)read;
        }
        ss >> itemChange.enableDyes;

        std::string read;
        while (ss >> read && read != "END") {}

        itemChanges.push_back(itemChange);
    }
}

void SkinChanger::Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, allocator_fns, toolbox_dll);
    GW::Initialize();

    minipetTransmogs.push_back(MinipetTransmog{36651, 350, 0x564EB, 12, 0x3d67, 0x40d97, 98820});

    GW::StoC::RegisterPostPacketCallback<GW::Packet::StoC::InstanceLoadFile>(&InstanceLoadFile_Entry, [this](GW::HookStatus*, const GW::Packet::StoC::InstanceLoadFile*) {
        forEachItem([&](GW::Item* item) 
        {
            const auto it = std::ranges::find_if(itemChanges, [&item](const auto& itemChange) 
            {
                return itemChange.item.modelID && itemChange.item.modelID == item->model_id && compareMods(itemChange.item.modifiers, item->mod_struct, item->mod_struct_size);
            });

            if (it != itemChanges.end()) {
                if (const auto modelFileID = toInt(it->modelFileID); modelFileID && modelFileID.value() != 0) {
                    item->model_file_id = *modelFileID;
                }
                if (it->enableDyes) {
                    item->dye.dye_tint = it->tint;
                    item->dye.dye1 = it->dyes[0];
                    item->dye.dye2 = it->dyes[1];
                    item->dye.dye3 = it->dyes[2];
                    item->dye.dye4 = it->dyes[3];
                }
            }
            return false;
        }, Behaviour::EquippableOnly);

        forEachItem([&](GW::Item* item) 
        {
            const auto it = std::ranges::find_if(minipetTransmogs, [&item](const auto& minipetTransmog) 
            {
                return minipetTransmog.itemToReplaceModelID && item->model_id == minipetTransmog.itemToReplaceModelID;
            });

            if (it != minipetTransmogs.end()) 
            {
                item->model_file_id = it->replacementItemModelFileID;
            }
            return false;
        }, Behaviour::AllItems);
    });

    GW::Chat::CreateCommand(L"restore", [](GW::HookStatus* status, const wchar_t*, const int argc, const LPWSTR* argv) {
        const auto instance = static_cast<SkinChanger*>(ToolboxPluginInstance());
        if (!instance || argc < 2) {
            status->blocked = false;
            return;
        }
        const auto arg1 = PluginUtils::ToLower(argv[1]);
        const auto pluginName = PluginUtils::StringToWString(instance->Name());

        std::filesystem::path iniToLoad;
        if (arg1 != PluginUtils::ToLower(pluginName)) {
            status->blocked = false;
            return;
        }
        if (argc < 3 || PluginUtils::ToLower(argv[2]) == L"recent") {
            PluginUtils::logMessage("Restore most recent backup", instance->Name());
            iniToLoad = BackupManager::getInstance().load(pluginName, BackupManager::LoadType::Latest);
        }
        else if (PluginUtils::ToLower(argv[2]) == L"largest") {
            PluginUtils::logMessage("Restore largest backup", instance->Name());
            iniToLoad = BackupManager::getInstance().load(pluginName, BackupManager::LoadType::Largest);
        }
        else if (PluginUtils::ToLower(argv[2]) == L"list") {
            PluginUtils::logMessage("Available backups:", instance->Name());
            const auto paths = BackupManager::getInstance().list(pluginName);
            for (const auto& path : paths) {
                const auto name = path.filename().string().substr(0, 1);
                const auto time = std::format("{:%Y-%m-%d %H:%M}", std::filesystem::last_write_time(path));
                const auto size = std::filesystem::file_size(path);
                PluginUtils::logMessage(std::format("Backup {}, Last change {}, File size {}", name, time, size), instance->Name());
            }
        }
        else if (PluginUtils::ToLower(argv[2]) == L"help") {
            PluginUtils::logMessage("Type \"/restore " + std::string{instance->Name()} + " recent\" to restore the most recent backup", instance->Name());
            PluginUtils::logMessage("Type \"/restore " + std::string{instance->Name()} + " largest\" to restore the largest backup", instance->Name());
            PluginUtils::logMessage("Type \"/restore " + std::string{instance->Name()} + " list\" to show the available backups", instance->Name());
            PluginUtils::logMessage("Type \"/restore " + std::string{instance->Name()} + " $NUMBER\" to restore a specific backup", instance->Name());
            PluginUtils::logMessage("Type \"/restore " + std::string{instance->Name()} + " help\" to show this menu", instance->Name());
        }
        else {
            try {
                const auto index = std::stoi(argv[2]);
                PluginUtils::logMessage("Restore backup " + std::to_string(index), instance->Name());
                iniToLoad = BackupManager::getInstance().load(pluginName, BackupManager::LoadType::Index, index);
            } catch (...) {
                status->blocked = false;
                return;
            }
        }
        if (!iniToLoad.empty()) {
            instance->loadFromIniFile(iniToLoad.c_str());
        }
    });
    GW::Chat::CreateCommand(L"skinchanger", [](GW::HookStatus*, const wchar_t*, const int argc, const LPWSTR* argv) 
        {
        const auto instance = static_cast<SkinChanger*>(ToolboxPluginInstance());
        if (!instance || argc < 2) return;

        const auto currentTarget = GW::Agents::GetTargetAsAgentLiving();
        if (currentTarget && currentTarget->IsNPC() && argc >= 3 && PluginUtils::ToLower(argv[1]) == L"save") 
        {
            const auto npc = GW::Agents::GetNPCByID(currentTarget->player_number);
            if (npc && npc->files_count) 
            {
                instance->npcTransmogs.push_back({PluginUtils::WStringToString(argv[2]), currentTarget->player_number, 0x64000000, npc->model_file_id, npc->model_files[0], npc->npc_flags});
            }
        }
    });

    RegisterUIMessageCallback(&UseItem_Entry, GW::UI::UIMessage::kSendUseItem, [&](GW::HookStatus*, GW::UI::UIMessage, void* wparam, void*) {
        if (!wparam) return;

        const auto item = GW::Items::GetItemById((uint32_t)wparam);
        if (!item) return;

        for (const auto& minipetTransmog : minipetTransmogs) 
        {
            if (item->model_id == minipetTransmog.itemToReplaceModelID) 
            {
                pendingMinipetTransmog = minipetTransmog;
                return;
            }
        }
    });

    GW::StoC::RegisterPostPacketCallback<GW::Packet::StoC::AgentAdd>(&AgentAdd_Entry, [&](GW::HookStatus*, const GW::Packet::StoC::AgentAdd* packet) {
        if (!pendingMinipetTransmog) return;
        const auto agent = GW::Agents::GetAgentByID(packet->agent_id);
        if (!agent || !agent->GetIsLivingType()) return;
        if (agent->GetAsAgentLiving()->player_number != pendingMinipetTransmog->agentToReplaceModelID) return;

        TransmoAgent(agent->agent_id, std::move(*pendingMinipetTransmog));
        pendingMinipetTransmog = std::nullopt;
    });
}

bool SkinChanger::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0 && ToolboxPlugin::CanTerminate();
}

void SkinChanger::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();
    GW::StoC::RemovePostCallback<GW::Packet::StoC::InstanceLoadFile>(&InstanceLoadFile_Entry);
    GW::DisableHooks();
}

void SkinChanger::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::Terminate();
}
