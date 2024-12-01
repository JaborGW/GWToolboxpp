#include <LeechSignetCancel.h>

#include <GWCA/GWCA.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Utilities/Hooker.h>

#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/UIMgr.h>

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static LeechSignetCancel instance;
    return &instance;
}

namespace 
{
    GW::HookEntry mindblade_hook;
}
void LeechSignetCancel::Initialize(ImGuiContext* ctx, ImGuiAllocFns fns, HMODULE toolbox_dll) {

    ToolboxPlugin::Initialize(ctx, fns, toolbox_dll);
    GW::Initialize();

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValueTarget>(&mindblade_hook, [this](GW::HookStatus* status, GW::Packet::StoC::GenericValueTarget* packet) -> void {
        UNREFERENCED_PARAMETER(status);

        if (packet->Value_id != GW::Packet::StoC::GenericValueID::skill_activated) return;
        if (static_cast<GW::Constants::SkillID>(packet->value) != GW::Constants::SkillID::Leech_Signet) return;

        const auto player = GW::Agents::GetControlledCharacter();
        if (player && player->skill) 
            GW::GameThread::Enqueue([] {
                GW::UI::Keypress(GW::UI::ControlAction_CancelAction);
            });
    });
}
void LeechSignetCancel::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();

    GW::StoC::RemoveCallback<GW::Packet::StoC::GenericValueTarget>(&mindblade_hook);
    GW::DisableHooks();
}
bool LeechSignetCancel::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void LeechSignetCancel::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::Terminate();
}
