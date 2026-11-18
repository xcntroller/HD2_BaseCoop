#pragma once

#include <memory>

class C_CampaignPreparationLive;
class C_Cursor;

class C_CampaignPreparationNativeUI {
public:
    explicit C_CampaignPreparationNativeUI(C_CampaignPreparationLive &model, int campaign = 0, int mission = 1, bool allowSpectators = false);
    ~C_CampaignPreparationNativeUI();
    bool IsUsable() const;
    void Update(int timeDelta);
    bool LeaveRequested() const;
    void KeyInput(float *pressed);
    void MouseInput(C_Cursor &cursor, int timeDelta);
    void Render();

private:
    struct S_View;
    std::unique_ptr<S_View> m_View;
};
