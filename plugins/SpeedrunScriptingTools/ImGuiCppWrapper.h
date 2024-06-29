#include <string>
#include <imgui.h>
#include <functional>
namespace ImGui 
{
    bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
    void ShowHelp(const char* help);


}
