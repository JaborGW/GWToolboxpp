#include "RawDialogs.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/GWCA.h>
#include <GWCA/Utilities/Hooker.h>

namespace {
    GW::HookEntry OnSentChat_HookEntry;

    void initializeCtos() 
    {
        GW::CtoS::Init();
        GW::CtoS::EnableHooks();
    }
    void sendDialog(DWORD dialogId)
    {
        #define GAME_CMSG_SEND_DIALOG (0x0039) // 57
        GW::CtoS::SendPacket(0x8, GAME_CMSG_SEND_DIALOG, dialogId);
    }

    std::string WStringToString(const std::wstring_view str)
    {
        // @Cleanup: ASSERT used incorrectly here; value passed could be from anywhere!
        if (str.empty()) {
            return "";
        }
        // NB: GW uses code page 0 (CP_ACP)
        const int try_code_pages[] = {CP_UTF8, CP_ACP};
        for (auto cp : try_code_pages) {
            const auto size_needed = WideCharToMultiByte(cp, WC_ERR_INVALID_CHARS, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
            if (!size_needed) continue;
            std::string dest(size_needed, 0);
            WideCharToMultiByte(cp, 0, str.data(), static_cast<int>(str.size()), dest.data(), size_needed, nullptr, nullptr);
            return dest;
        }
        return {};
    }

    void OnSendChat(GW::HookStatus* status, GW::UI::UIMessage message_id, void* wparam, void*)
    {
        constexpr auto rawDialogStart = "/rawdialog ";
        constexpr auto rawDialogStartLength = std::string_view(rawDialogStart).size();

        if (message_id != GW::UI::UIMessage::kSendChatMessage) return;
        const auto wmessage = static_cast<GW::UI::UIPacket::kSendChatMessage*>(wparam)->message;
        if (!(wmessage && *wmessage)) return;
        const auto channel = GW::Chat::GetChannel(*wmessage);
        if (channel != GW::Chat::CHANNEL_COMMAND || status->blocked) return;
        
        const auto message = WStringToString(wmessage);
        if (!message.starts_with(rawDialogStart)) return;
        
        const auto dialogString = message.substr(rawDialogStartLength);
        if (dialogString.empty()) return;
        const auto dialogId = std::stoi(dialogString, nullptr, 0);
        status->blocked = true;
        sendDialog(dialogId);
    }
} // namespace

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static RawDialogs instance;
    return &instance;
}

void RawDialogs::DrawSettings()
{
    ToolboxPlugin::DrawSettings();

    ImGui::Text("Version 1.0. For new releases, feature requests and bug reports check out");
    ImGui::SameLine();

    constexpr auto discordInviteLink = "https://discord.gg/ZpKzer4dK9";
    ImGui::TextColored(ImColor{102, 187, 238, 255}, discordInviteLink);
    if (ImGui::IsItemClicked()) {
        ShellExecute(nullptr, "open", discordInviteLink, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void RawDialogs::Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, allocator_fns, toolbox_dll);
    GW::Initialize();
    GW::CtoS::Init();
    GW::UI::RegisterUIMessageCallback(&OnSentChat_HookEntry, GW::UI::UIMessage::kSendChatMessage, OnSendChat);
}

bool RawDialogs::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0 && GW::CtoS::HookCount() == 0 && ToolboxPlugin::CanTerminate();
}

void RawDialogs::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();
    GW::UI::RemoveUIMessageCallback(&OnSentChat_HookEntry, GW::UI::UIMessage::kSendChatMessage);
    GW::DisableHooks();
    GW::CtoS::DisableHooks();
}

void RawDialogs::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::CtoS::Exit();
    GW::Terminate();
}
