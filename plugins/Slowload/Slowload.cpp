#include "Slowload.h"

#include <GWCA/GWCA.h>

#include <GWCA/Context/CharContext.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Constants/Constants.h>

#include <Utils/GuiUtils.h>

#include <Keys.h>

#include <SimpleIni.h>
#include <chrono>
#include <filesystem>
#include <imgui.h>
#include <thread>

namespace {}
DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static Slowload instance;
    return &instance;
}

void Slowload::SignalTerminate()
{
    ToolboxUIPlugin::Terminate();
    GW::DisableHooks();
}
bool Slowload::CanTerminate()
{
    return ToolboxUIPlugin::CanTerminate() && GW::HookBase::GetInHookCount() == 0;
}

void Slowload::Terminate()
{
    ToolboxUIPlugin::Terminate();
    GW::Terminate();
}

void Slowload::Draw(IDirect3DDevice9*)
{
    if (GW::Map::GetInstanceType() != GW::Constants::InstanceType::Outpost || !is_active) return;

    constexpr auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    if (ImGui::Begin(Name(), can_close && show_closebutton ? GetVisiblePtr() : nullptr, GetWinFlags(flags))) {
        ImGui::PushFont(GetFont(GuiUtils::FontSize::widget_large));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
        ImGui::Text("Slowload active");
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
    ImGui::End();
}

namespace {
    //Forget you ever saw this
    std::string to_string(const std::wstring& wstring) {
        return std::filesystem::path(wstring).string();
    }
    std::wstring to_wstring(const std::string& string)
    {
        return std::filesystem::path(string).wstring();
    }

}

void Slowload::LoadSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::LoadSettings(folder);

    ini.LoadFile(GetSettingFile(folder).c_str());
    shortcutKey = ini.GetLongValue(Name(), VAR_NAME(shortcutKey), shortcutKey);
    shortcutMod = ini.GetLongValue(Name(), VAR_NAME(shortcutMod), shortcutMod);

    slowloading_character_name = to_wstring(ini.GetValue(Name(), VAR_NAME(slowloading_character_name), ""));
    
    ModKeyName(hotkeyDescription, _countof(hotkeyDescription), shortcutMod, shortcutKey);
}

void Slowload::SaveSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    ini.SetLongValue(Name(), VAR_NAME(shortcutKey), shortcutKey);
    ini.SetLongValue(Name(), VAR_NAME(shortcutMod), shortcutMod);
    ini.SetValue(Name(), VAR_NAME(slowloading_character_name), to_string(slowloading_character_name).c_str());

    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}

void Slowload::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings();

    ImGui::Text("Hotkey:       ");
    ImGui::SameLine();
    if (ImGui::Button(hotkeyDescription)) {
        ImGui::OpenPopup("Select Hotkey");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to change hotkey");
    if (ImGui::BeginPopup("Select Hotkey")) {
        static long newkey = 0;
        char newkey_buf[256]{};
        long newmod = 0;

        ImGui::Text("Press key");

        // Modifiers are causing crashes for some reason
        /* if (ImGui::IsKeyDown(ImGuiKey_ModCtrl))newmod |= ModKey_Control;
        if (ImGui::IsKeyDown(ImGuiKey_ModShift)) newmod |= ModKey_Shift;
        if (ImGui::IsKeyDown(ImGuiKey_ModAlt)) newmod |= ModKey_Alt;*/

        if (newkey == 0) { // we are looking for the key
            for (WORD i = 0; i < 512; i++) {
                switch (i) {
                    case VK_CONTROL:
                    case VK_LCONTROL:
                    case VK_RCONTROL:
                    case VK_SHIFT:
                    case VK_LSHIFT:
                    case VK_RSHIFT:
                    case VK_MENU:
                    case VK_LMENU:
                    case VK_RMENU:
                        continue;
                    default: {
                        if (::GetKeyState(i) & 0x8000) newkey = i;
                    }
                }
            }
        }
        else if (!(::GetKeyState(newkey) & 0x8000)) { // We have a key, check if it was released
            shortcutKey = newkey;
            shortcutMod = newmod;
            ModKeyName(hotkeyDescription, _countof(hotkeyDescription), newmod, newkey);
            newkey = 0;
            ImGui::CloseCurrentPopup();
        }

        ModKeyName(newkey_buf, _countof(newkey_buf), newmod, newkey);
        ImGui::Text("%s", newkey_buf);

        ImGui::EndPopup();
    }

    if (slowloading_character_name.empty()) {
        if (ImGui::Button("Restrict slowloading to current character")) 
            slowloading_character_name = GW::GetCharContext()->player_name;
    }
    else {
        ImGui::Text("Slowloading restricted to: %ls", &slowloading_character_name[0]);
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            slowloading_character_name.clear();
        }
    }
}

namespace {
    std::string WStringToString(const std::wstring_view str)
    {
        // @Cleanup: ASSERT used incorrectly here; value passed could be from anywhere!
        if (str.empty()) {
            return "";
        }
        // NB: GW uses code page 0 (CP_ACP)
        const auto size_needed = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
        std::string str_to(size_needed, 0);
        return std::move(str_to);
    }
}

void Slowload::Initialize(ImGuiContext* ctx, ImGuiAllocFns fns, HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
    GW::Initialize();

    auto keyIsPressed = [&] {
        bool result = GetAsyncKeyState(shortcutKey) & (1 << 15);
        if (shortcutMod & ModKey_Control) result &= ImGui::IsKeyDown(ImGuiKey_ModCtrl);
        if (shortcutMod & ModKey_Shift) result &= ImGui::IsKeyDown(ImGuiKey_ModShift);
        if (shortcutMod & ModKey_Alt) result &= ImGui::IsKeyDown(ImGuiKey_ModAlt);
        return result;
    };

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::InstanceLoadFile>(&instanceLoadEntry, [this, keyIsPressed](GW::HookStatus*, GW::Packet::StoC::InstanceLoadFile*) {
        using namespace std::chrono_literals;
        if (!is_active) return;
        if (!slowloading_character_name.empty() && slowloading_character_name != GW::GetCharContext()->player_name) return;

        is_active = false;
        std::this_thread::sleep_for(3s);
        while (!keyIsPressed())
            std::this_thread::sleep_for(100ms);
    });
    instanceLoadEntry;
}

void Slowload::Update(float delta)
{
    ToolboxUIPlugin::Update(delta);

    if (GW::Map::GetInstanceType() != GW::Constants::InstanceType::Outpost) return;

    bool keyIsPressed = GetAsyncKeyState(shortcutKey) & (1 << 15);
    if (shortcutMod & ModKey_Control) keyIsPressed &= ImGui::IsKeyDown(ImGuiKey_ModCtrl);
    if (shortcutMod & ModKey_Shift) keyIsPressed &= ImGui::IsKeyDown(ImGuiKey_ModShift);
    if (shortcutMod & ModKey_Alt) keyIsPressed &= ImGui::IsKeyDown(ImGuiKey_ModAlt);

    if (keyIsPressed && !keyWasPressed) {
        is_active = !is_active;
    }
    keyWasPressed = keyIsPressed;
}
