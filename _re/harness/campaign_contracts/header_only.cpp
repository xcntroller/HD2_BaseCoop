// Must compile without a precompiled header, engine headers, or game libraries.
#include "../../../src/game/campaign_contracts.h"
#ifdef _MSC_VER
static_assert(sizeof(void *) == 4 && _M_IX86, "MSVC target must be x86");
#endif
S_CampaignPreparationView CampaignHeaderOnlyProbe() { return {}; }
