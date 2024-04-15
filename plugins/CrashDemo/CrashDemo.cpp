#include "CrashDemo.h"

#include <GWCA/GWCA.h>

#include <GWCA/Utilities/Hooker.h>

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static CrashDemo instance;
    return &instance;
}

void CrashDemo::Initialize(ImGuiContext* ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Initialize();
}

void CrashDemo::SignalTerminate()
{
    GW::DisableHooks();
}

bool CrashDemo::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void CrashDemo::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::Terminate();
}
