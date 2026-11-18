#pragma once

#include <memory>

class C_CampaignPreparationLive;
class C_Cursor;

class C_CampaignPreparationNativeUI {
public:
    explicit C_CampaignPreparationNativeUI(C_CampaignPreparationLive &model);
    ~C_CampaignPreparationNativeUI();
    bool IsUsable() const;
    void Update();
    bool LeaveRequested() const;
    void KeyInput(float *pressed);
    void MouseInput(C_Cursor &cursor, int timeDelta);
    void Render();

private:
    struct S_View;
    std::unique_ptr<S_View> m_View;
};
