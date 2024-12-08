#pragma once

#include <ToolboxUIPlugin.h>
#include <imgui.h>

class MinipetPopIndicator : public ToolboxUIPlugin {
public:
    MinipetPopIndicator();
    ~MinipetPopIndicator() override = default;

    const char* Name() const override { return "MinipetPopIndicator"; }
    const char* Icon() const override { return ICON_FA_CIRCLE; }

    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;
    void DrawSettings() override;
    void Draw(IDirect3DDevice9* pDevice) override;

    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void Terminate() override;

private:
    float radius = 50.f;
    ImVec4 color = {0.247f, 0.282f, 0.8f, 1.f};
    bool showIcon = true;
    bool showText = false;
};
