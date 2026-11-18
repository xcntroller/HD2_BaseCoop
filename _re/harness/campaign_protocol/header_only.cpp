#include "../../../src/net/campaign_protocol.h"
#ifdef _MSC_VER
static_assert(sizeof(void *) == 4, "required MSVC x86 target");
#ifndef _M_IX86
#error Intended target is MSVC x86
#endif
#endif
static_assert(CampaignProtocol::kCommand == 0x434f4f50);
static_assert(std::variant_size<CampaignProtocol::T_Message>::value == 24);
