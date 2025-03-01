#pragma once

#include <ToolboxUIPlugin.h>

namespace GW 
{
    struct ItemModifier;
}
struct InventoryItem {
    uint32_t modelID = 0;
    std::wstring encodedName;
    std::vector<GW::ItemModifier> modifiers = {};
};

class SkinChanger : public ToolboxPlugin {
public:
    SkinChanger() {}
    ~SkinChanger() override = default;

    const char* Name() const override { return "SkinChanger"; }
    const char* Icon() const override { return ICON_FA_DICE; }

    void DrawSettings() override;
    bool HasSettings() const override { return true; }
    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;

    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    bool CanTerminate() override;
    void SignalTerminate() override;
    void Terminate() override;

private:
    std::vector<std::pair<InventoryItem, std::string>> changedItems;
};
