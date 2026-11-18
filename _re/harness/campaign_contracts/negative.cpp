#include "../../../src/game/campaign_contracts.h"

// Each case MUST fail compilation; run.py checks it independently on each compiler.
#if CAMPAIGN_NEGATIVE_CASE == 1
SoldierId bad = PlayerId{1};
#elif CAMPAIGN_NEGATIVE_CASE == 2
PlayerId bad = ActorId{0};
#elif CAMPAIGN_NEGATIVE_CASE == 3
PreparationRevision bad = ViewRevision{1};
#elif CAMPAIGN_NEGATIVE_CASE == 4
void Bad(T_CampaignPreparationSnapshot snapshot) { snapshot->m_Stock.clear(); }
#elif CAMPAIGN_NEGATIVE_CASE == 5
void Bad(T_CampaignPreparationSnapshot snapshot) { snapshot->m_Loadouts[0].m_Items[0].m_State.m_Quantity = 99; }
#elif CAMPAIGN_NEGATIVE_CASE == 6
void Bad(I_CampaignPreparation &api) { api.SubmitEdit(S_CampaignOwnershipRequest{}); }
#else
#error Define CAMPAIGN_NEGATIVE_CASE in [1, 6]
#endif
