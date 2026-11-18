#pragma once

#include <memory>

#include "game/campaign_ownership.h"
#include "game/campaign_preparation_live.h"
#include "net/campaign_net.h"

struct S_CampaignNativeCatalog;

enum class E_CampaignSetupState { Idle, Joining, Hosting, Guest };

bool CampaignSetupMenuEnabled();
std::optional<std::uint32_t> ParseCampaignLanAddress(const char *text);

// The setup screen owns this game-thread object through cancellation or exit.
// Future preparation may move its owning pointer while retaining the same
// session identity, admission callbacks, ownership and request journal.
class C_CampaignSetupSession final {
public:
    C_CampaignSetupSession();
    ~C_CampaignSetupSession();
    C_CampaignSetupSession(const C_CampaignSetupSession &) = delete;
    C_CampaignSetupSession &operator=(const C_CampaignSetupSession &) = delete;

    E_CampaignReason Host();
    E_CampaignReason Join(std::uint32_t address);
    E_CampaignReason BeginPreparation();
    void Update();
    void Close();

    E_CampaignSetupState GetState() const { return m_State; }
    E_CampaignReason GetJoinFailure() const { return m_JoinFailure; }
    C_CampaignOwnership *GetHostOwnership() const { return m_Ownership.get(); }
    C_CampaignRequestJournal *GetHostJournal() const { return m_Journal.get(); }
    const std::optional<CampaignProtocol::S_Accept> &GetGuestAgreement() const { return m_Agreement; }
    C_CampaignPreparationLive *GetPreparation() const { return m_Preparation.get(); }
    const S_CampaignNativeCatalog *GetPreparationCatalog() const { return m_Catalog.get(); }

private:
    class C_SetupPolicy final : public I_CampaignPolicyView {
    public:
        S_CampaignPolicySnapshot GetSnapshot() const override {
            return {{{E_CampaignPolicy::SoldierSelection, E_CampaignPolicyState::Permitted, {}}}};
        }
    };

    E_CampaignSetupState m_State = E_CampaignSetupState::Idle;
    E_CampaignReason m_JoinFailure = E_CampaignReason::None;
    C_SetupPolicy m_Policy;
    std::unique_ptr<C_CampaignRequestJournal> m_Journal;
    std::unique_ptr<C_CampaignOwnership> m_Ownership;
    std::optional<CampaignProtocol::S_Accept> m_Agreement;
    std::unique_ptr<C_CampaignPreparationLive> m_Preparation;
    std::unique_ptr<S_CampaignNativeCatalog> m_Catalog;
};
