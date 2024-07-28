#pragma once
#include <IconsFontAwesome5.h>

#include <ToolboxModule.h>

class QuestModule : public ToolboxModule {
public:
    static QuestModule& Instance()
    {
        static QuestModule instance;
        return instance;
    }

    [[nodiscard]] const char* Name() const override { return "Quest Module"; }
    [[nodiscard]] const char* Icon() const override { return ICON_FA_COMPASS; }
    [[nodiscard]] const char* Description() const override { return "A set of QoL improvements to the quest log and related behavior"; }

    void DrawSettingsInternal() override;
    void LoadSettings(ToolboxIni*) override;
    void SaveSettings(ToolboxIni*) override;
    void Initialize() override;
    void Terminate() override;
    void SignalTerminate() override;
    void Update(float) override;

    static ImU32 GetQuestColor(GW::Constants::QuestID);
};
