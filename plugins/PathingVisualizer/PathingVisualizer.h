#pragma once

#include <ToolboxUIPlugin.h>

class PathingVisualizer : public ToolboxUIPlugin {
public:
    PathingVisualizer() = default;
    ~PathingVisualizer() override = default;

    const char* Name() const override { return "PathingVisualizer"; }

    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;

    void DrawSettings() override;
    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;

    void Draw(IDirect3DDevice9*) override;
    void Update(float) override;
};
