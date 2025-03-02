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

private:
    void loadFromIniFile(const wchar_t* filePath);
    std::vector<ItemChange> itemChanges;
};
