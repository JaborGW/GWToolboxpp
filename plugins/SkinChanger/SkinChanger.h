#pragma once

#include <ToolboxUIPlugin.h>

namespace GW 
{
    struct ItemModifier;
    enum class DyeColor : uint8_t;
}
struct InventoryItem 
{
    uint32_t modelID = 0;
    std::wstring encodedName;
    std::vector<GW::ItemModifier> modifiers = {};
};

struct ItemChange 
{
    InventoryItem item;
    std::string modelFileID = "0x";

    bool enableDyes = false;
    std::array<GW::DyeColor, 4> dyes;
    uint8_t tint = 255;
};

struct NpcTransmog 
{
    std::string identifier;
    DWORD npc_id = 0;
    DWORD scale = 0x64000000;
    DWORD npc_model_file_id = 0;
    DWORD npc_model_file_data = 0;
    DWORD flags = 0;
};

class SkinChanger : public ToolboxPlugin {
public:
    SkinChanger() {}
    ~SkinChanger() override = default;

    const char* Name() const override { return "SkinChanger"; }
    const char* Icon() const override { return ICON_FA_PEOPLE_ARROWS; }

    void DrawSettings() override;
    bool HasSettings() const override { return true; }
    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;

    void Update(float) override;

    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    bool CanTerminate() override;
    void SignalTerminate() override;
    void Terminate() override;

    void loadFromIniFile(const wchar_t* filePath);

    std::vector<ItemChange> itemChanges;
    std::vector<NpcTransmog> npcTransmogs;
};
