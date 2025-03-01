#include "SkinChanger.h"

#include <cstdint>

#include <GWCA/GWCA.h>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Packets/StoC.h>

#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/AgentMgr.h>

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Utilities/Hooker.h>

#include "PluginUtils.h"

#include <sstream>

namespace 
{
    bool mapChangeTriggered = false;
    std::map<GW::Constants::BagType, std::vector<InventoryItem>> available_items;
    std::unordered_map<std::wstring, std::wstring> decodedNames;

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

    void forEachEquippableItem(std::function<bool(GW::Item*)> func) 
    {
        using GW::Constants::Bag;
        for (auto bagSlot : {Bag::Backpack, Bag::Belt_Pouch, Bag::Bag_1, Bag::Bag_2, Bag::Equipment_Pack, Bag::Equipped_Items}) 
        {
            const auto bag = GW::Items::GetBag(bagSlot);
            if (!bag) continue;

            const auto& items = bag->items;
            for (size_t slot = 0; slot < items.size(); slot++) 
            {
                const auto item = items[slot];
                if (IsEquippable(items[slot])) 
                {
                    if (func(item)) return;
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
            ImGui::OpenPopup("Choose item to equip");
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
            forEachEquippableItem([&inventoryItem](const GW::Item* item) 
            {
                if (item->model_id == inventoryItem.modelID && compareMods(inventoryItem.modifiers, item->mod_struct, item->mod_struct_size)) 
                {
                    inventoryItem.encodedName = item->single_item_name;
                    return true;
                }
                return false;
            });
        }
        
        if (ImGui::BeginPopupModal("Choose item to equip", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            if (needToFetchBagItems) 
            {
                available_items.clear();
                forEachEquippableItem([&](const GW::Item* item)
                {
                    available_items[item->bag->bag_type].push_back(InventoryItem{item->model_id, item->single_item_name, extractMods(item)});
                    return false;
                });
                needToFetchBagItems = false;
            }
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
            for (auto& [bag, bagItems] : available_items) 
            {
                ImGui::TextUnformatted(bags[(uint32_t)bag]);
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
} // namespace

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static SkinChanger instance;
    return &instance;
}

void SkinChanger::LoadSettings(const wchar_t* folder)
{
    ToolboxPlugin::LoadSettings(folder);
    ini.LoadFile(GetSettingFile(folder).c_str());

    std::string loadedItems = ini.GetValue(Name(), VAR_NAME(itemChanges), "");
    enableItemColoring = ini.GetBoolValue(Name(), VAR_NAME(enableItemColoring), enableItemColoring);
    if (loadedItems.empty()) return;

    std::stringstream ss{loadedItems};

    while (ss) 
    {
        ItemChange itemChange;

        ss >> std::ws >> itemChange.item.modelID >> std::ws;

        uint32_t readMod;
        while (ss && ss.peek() != 'S') 
        {
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
        for (auto& dye : itemChange.dyes) 
        {
            ss >> readDye;
            dye = (GW::DyeColor)(readDye);
        }
        itemChanges.push_back(itemChange);
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
    }

    ini.SetValue(Name(), VAR_NAME(itemChanges), itemsToSave.c_str());
    ini.SetBoolValue(Name(), VAR_NAME(enableItemColoring), enableItemColoring);
    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}

void SkinChanger::DrawSettings()
{
    ToolboxPlugin::DrawSettings();

    int index = 0;
    std::optional<int> indexToDelete;
    for (auto& itemChange : itemChanges) {
        ImGui::PushID(index++);

        if (ImGui::Button("X")) indexToDelete = index - 1;
        ImGui::SameLine();

        drawItemSelector(itemChange.item);
        ImGui::SameLine();
       
        ImGui::PushItemWidth(100.f);
        ImGui::InputText("", &itemChange.modelFileID);
        std::erase_if(itemChange.modelFileID, [](auto c){return std::isspace(c);});
        ImGui::PopItemWidth();

        if (enableItemColoring) {
            ImGui::SameLine();
            drawDyePicker("Dye 1", &itemChange.dyes[0]);
            ImGui::SameLine();
            drawDyePicker("Dye 2", &itemChange.dyes[1]);
            ImGui::SameLine();
            drawDyePicker("Dye 3", &itemChange.dyes[2]);
            ImGui::SameLine();
            drawDyePicker("Dye 4", &itemChange.dyes[3]);
        }

        ImGui::PopID();
    }
    if (ImGui::Button("+")) 
    {
        itemChanges.push_back({{}, "0x", {GW::DyeColor::None, GW::DyeColor::None, GW::DyeColor::None, GW::DyeColor::None}});
    }
    if (indexToDelete) itemChanges.erase(itemChanges.begin() + *indexToDelete);

    ImGui::Checkbox("Enable dyes", &enableItemColoring);
    ImGui::Text("Item changes are applied on instance load.");
    // -----------

    ImGui::Text("Version 1.0-beta2. For new releases, feature requests and bug reports check out");
    ImGui::SameLine();

    constexpr auto discordInviteLink = "https://discord.gg/ZpKzer4dK9";
    ImGui::TextColored(ImColor{102, 187, 238, 255}, discordInviteLink);
    if (ImGui::IsItemClicked()) {
        ShellExecute(nullptr, "open", discordInviteLink, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void SkinChanger::Update(float)
{
    if (GW::Map::GetInstanceType() == GW::Constants::InstanceType::Loading)
    {
        mapChangeTriggered = true;
        return;
    }
    if (!mapChangeTriggered || GW::Map::GetInstanceType() == GW::Constants::InstanceType::Loading || !GW::Map::GetIsMapLoaded() || GW::Map::GetIsObserving()) 
    {
        return;
    }
    mapChangeTriggered = false;

    forEachEquippableItem([&](GW::Item* item) {
        const auto it = std::ranges::find_if(itemChanges, [&item](const auto& itemChange) 
        {
            return itemChange.item.modelID && itemChange.item.modelID == item->model_id && compareMods(itemChange.item.modifiers, item->mod_struct, item->mod_struct_size);
        });

        if (it != itemChanges.end()) 
        {
            if (const auto modelFileID = toInt(it->modelFileID); modelFileID && modelFileID.value() != 0) 
            {
                item->model_file_id = *modelFileID;
            }
            if (enableItemColoring) 
            {
                item->dye.dye1 = it->dyes[0];
                item->dye.dye2 = it->dyes[1];
                item->dye.dye3 = it->dyes[2];
                item->dye.dye4 = it->dyes[3];
            }
        }
        return false;
    });
}

void SkinChanger::Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, allocator_fns, toolbox_dll);
    GW::Initialize();
}

bool SkinChanger::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0 && ToolboxPlugin::CanTerminate();
}

void SkinChanger::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();
    GW::DisableHooks();
}

void SkinChanger::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::Terminate();
}
