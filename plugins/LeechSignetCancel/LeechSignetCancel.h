#pragma once

#include <ToolboxPlugin.h>
#include <GWCA/Utilities/Hook.h>

class LeechSignetCancel : public ToolboxPlugin {
public:
    LeechSignetCancel() = default;
    ~LeechSignetCancel() override = default;

    const char* Name() const override { return "Leech Signet Cancel"; }

    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;
};
