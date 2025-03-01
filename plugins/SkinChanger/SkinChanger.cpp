#include "SkinChanger.h"

#include <GWCA/GWCA.h>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Packets/StoC.h>

#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/StoCMgr.h>

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Utilities/Hooker.h>

#include "PluginUtils.h"

#include <sstream>

namespace 
{
    GW::HookEntry InstanceLoadFile_Entry;

    std::map<GW::Constants::BagType, std::vector<InventoryItem>> available_items;
    std::unordered_map<std::wstring, std::wstring> decodedNames;

    std::string decode(const std::wstring& wstring) 
    {
        if (wstring.empty()) return "";
        auto& decoded = decodedNames[wstring];
        if (decoded.empty()) GW::UI::AsyncDecodeStr(wstring.c_str(), &decoded);
        return PluginUtils::WStringToString(decoded);
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
        for (auto bagSlot = GW::Constants::Bag::Backpack; bagSlot <= GW::Constants::Bag::Equipment_Pack; bagSlot = (GW::Constants::Bag)((size_t)bagSlot + 1)) 
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
                if (item->model_id == inventoryItem.modelID && extractMods(item) == inventoryItem.modifiers) 
                {
                    inventoryItem.encodedName = item->name_enc;
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
                    available_items[item->bag->bag_type].push_back(InventoryItem{item->model_id, item->name_enc, extractMods(item)});
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
    std::string loadedItems = ini.GetValue(Name(), VAR_NAME(changedItems), "");

    if (loadedItems.empty()) return;

    std::stringstream ss{loadedItems};

    while (ss) 
    {
        InventoryItem item;
        std::string modelFileID;

        ss >> std::ws >> item.modelID >> std::ws;

        uint32_t readMod;
        while (ss && ss.peek() != 'S') 
        {
            ss >> readMod;
            item.modifiers.push_back({readMod});
            ss >> std::ws;
        }
        if (ss && ss.peek() == 'S')
            ss.get();
        else
            return;

        ss >> std::ws;
        if (ss)
            ss >> modelFileID;
        changedItems.push_back(std::make_pair(item, modelFileID));
    }
}

void SkinChanger::SaveSettings(const wchar_t* folder)
{
    ToolboxPlugin::SaveSettings(folder);
    
    std::string itemsToSave;
    itemsToSave.reserve(4096);
    for (const auto& [item, modelFileID] : changedItems) 
    {
        itemsToSave += std::to_string(item.modelID) + " ";
        for (const auto& mod : item.modifiers) 
        {
            itemsToSave += std::to_string(mod.mod) + " ";
        }
        itemsToSave += "S ";
        itemsToSave += modelFileID + " ";
    }

    ini.SetValue(Name(), VAR_NAME(changedItems), itemsToSave.c_str());
    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}

void SkinChanger::DrawSettings()
{
    ToolboxPlugin::DrawSettings();

    int index = 0;
    std::optional<int> indexToDelete;
    for (auto& [item, fileID] : changedItems) {
        ImGui::PushID(index++);

        if (ImGui::Button("X")) indexToDelete = index - 1;
        ImGui::SameLine();

        drawItemSelector(item);
        ImGui::SameLine();
       
        ImGui::PushItemWidth(150.f);
        ImGui::InputText("", &fileID);
        std::erase_if(fileID, [](auto c){return std::isspace(c);});
        ImGui::PopItemWidth();

        ImGui::PopID();
    }
    if (ImGui::Button("+")) 
    {
        changedItems.push_back(std::make_pair(InventoryItem{}, "0x"));
    }
    if (indexToDelete) changedItems.erase(changedItems.begin() + *indexToDelete);

    // -----------

    ImGui::Text("Version 1.0. For new releases, feature requests and bug reports check out");
    ImGui::SameLine();

    constexpr auto discordInviteLink = "https://discord.gg/ZpKzer4dK9";
    ImGui::TextColored(ImColor{102, 187, 238, 255}, discordInviteLink);
    if (ImGui::IsItemClicked()) {
        ShellExecute(nullptr, "open", discordInviteLink, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void SkinChanger::Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, allocator_fns, toolbox_dll);
    GW::Initialize();

    GW::StoC::RegisterPostPacketCallback<GW::Packet::StoC::InstanceLoadFile>(&InstanceLoadFile_Entry, [this](GW::HookStatus*, const GW::Packet::StoC::InstanceLoadFile*)
    {
        forEachEquippableItem([&](GW::Item* item) {
            const auto it = std::ranges::find_if(changedItems, [&item](const auto& changedItem) {
                return changedItem.first.modelID && changedItem.first.modelID == item->model_id && extractMods(item) == changedItem.first.modifiers;
            });

            if (it != changedItems.end()) {
                const auto modelFileID = toInt(it->second);
                if (modelFileID && modelFileID.value() != 0) item->model_file_id = *modelFileID;
            }
            return false;
        });
    });
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
