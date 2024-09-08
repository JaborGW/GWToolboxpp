#pragma once

#include <ToolboxUIPlugin.h>

class FlatBowRangeIndicator : public ToolboxUIPlugin {
public:
    FlatBowRangeIndicator()
    {
    }
    ~FlatBowRangeIndicator() override = default;

    const char* Name() const override { return "FlatBowRangeIndicator"; }
    const char* Icon() const override { return ICON_FA_RAINBOW; }

    void DrawSettings() override;
    void Draw(IDirect3DDevice9* pDevice) override;
};
