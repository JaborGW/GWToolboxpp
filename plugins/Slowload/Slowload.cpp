#include "Slowload.h"

#include <GWCA/GWCA.h>

#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/StoC.h>

#include <Keys.h>

#include <imgui.h>
#include <SimpleIni.h>
#include <filesystem>
#include <chrono>
#include <thread>

namespace {

}

void Slowload::DrawSettings()
{
    ToolboxPlugin::DrawSettings();
    
}

void Slowload::LoadSettings(const wchar_t* folder)
{
    CSimpleIniA ini{};
    const auto path = std::filesystem::path(folder) / L"Slowload.ini";
    ini.LoadFile(path.wstring().c_str());
}

void Slowload::SaveSettings(const wchar_t* folder)
{
    CSimpleIniA ini{};
    const auto path = std::filesystem::path(folder) / L"Slowload.ini";

    ini.SaveFile(path.wstring().c_str());
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static Slowload instance;
    return &instance;
}
void Slowload::Update(float delta)
{
    ToolboxPlugin::Update(delta);

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::InstanceLoadFile>(&instanceLoadEntry, [this](GW::HookStatus*, GW::Packet::StoC::InstanceLoadFile*) -> void {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(350s);
    });
}

void Slowload::Initialize(ImGuiContext* ctx, ImGuiAllocFns fns, HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, fns, toolbox_dll);
    GW::Initialize();
}
void Slowload::SignalTerminate() 
{ 
    GW::DisableHooks(); 
}
bool Slowload::CanTerminate() 
{ 
    return GW::HookBase::GetInHookCount() == 0; 
}

void Slowload::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::Terminate();
}
