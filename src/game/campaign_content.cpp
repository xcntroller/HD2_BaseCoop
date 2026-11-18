#include <windows.h>
#include <wincrypt.h>

#include "game/campaign_content.h"

#include <algorithm>
#include <array>

#include "ls3d/rw_data.h"
#include "net/campaign_net.h"

namespace {

using T_Digest = std::array<BYTE, 32>;

class C_Hash {
public:
    explicit C_Hash(HCRYPTPROV provider) { CryptCreateHash(provider, CALG_SHA_256, 0, 0, &m_Hash); }
    ~C_Hash() { if (m_Hash) CryptDestroyHash(m_Hash); }
    bool Write(const void *data, DWORD size)
    {
        return m_Hash && CryptHashData(m_Hash, static_cast<const BYTE *>(data), size, 0);
    }
    bool WriteNumber(std::uint64_t value, DWORD size)
    {
        BYTE bytes[8]{};
        for (DWORD i = 0; i < size; ++i)
            bytes[i] = static_cast<BYTE>(value >> (i * 8));
        return Write(bytes, size);
    }
    bool Finish(T_Digest &digest)
    {
        DWORD size = static_cast<DWORD>(digest.size());
        return m_Hash && CryptGetHashParam(m_Hash, HP_HASHVAL, digest.data(), &size, 0) && size == digest.size();
    }

private:
    HCRYPTHASH m_Hash = 0;
};

struct S_Content {
    std::string m_Path;
    std::string m_InlineData;
};

bool DigestContent(HCRYPTPROV provider, const S_Content &entry, std::uint64_t &size, T_Digest &digest)
{
    C_Hash hash(provider);
    if (!entry.m_InlineData.empty()) {
        size = entry.m_InlineData.size();
        return hash.Write(entry.m_InlineData.data(), static_cast<DWORD>(size)) && hash.Finish(digest);
    }

    std::string path = entry.m_Path;
    std::replace(path.begin(), path.end(), '/', '\\');
    const int file = dtaOpen(path.c_str(), false);
    if (file < 0)
        return false;
    const int length = dtaSeek(file, 0, 2);
    bool valid = length >= 0 && dtaSeek(file, 0, 0) == 0;
    std::array<BYTE, 32768> buffer;
    int remaining = length;
    while (valid && remaining > 0) {
        const int count = (std::min)(remaining, static_cast<int>(buffer.size()));
        valid = dtaRead(file, buffer.data(), count) == count && hash.Write(buffer.data(), static_cast<DWORD>(count));
        remaining -= count;
    }
    dtaClose(file);
    if (!valid)
        return false;
    size = static_cast<std::uint64_t>(length);
    return hash.Finish(digest);
}

std::string DigestCategory(HCRYPTPROV provider, unsigned category, std::vector<S_Content> entries)
{
    std::sort(entries.begin(), entries.end(), [](const S_Content &a, const S_Content &b) {
        return a.m_Path < b.m_Path;
    });
    constexpr char prefix[] = "HD2COOP-CONTENT-v1";
    C_Hash hash(provider);
    if (!hash.Write(prefix, sizeof(prefix)) || !hash.WriteNumber(category, 1) || !hash.WriteNumber(entries.size(), 4))
        return {};
    for (const auto &entry : entries) {
        std::uint64_t size = 0;
        T_Digest digest;
        if (!DigestContent(provider, entry, size, digest) ||
            !hash.WriteNumber(entry.m_Path.size(), 2) ||
            !hash.Write(entry.m_Path.data(), static_cast<DWORD>(entry.m_Path.size())) ||
            !hash.WriteNumber(size, 8) || !hash.Write(digest.data(), static_cast<DWORD>(digest.size())))
            return {};
    }
    T_Digest digest;
    if (!hash.Finish(digest))
        return {};
    constexpr char hex[] = "0123456789abcdef";
    std::string encoded = "sha256:";
    for (const auto byte : digest) {
        encoded += hex[byte >> 4];
        encoded += hex[byte & 15];
    }
    return encoded;
}

} // namespace

bool BuildCampaignNetConfig(const char *missionName, bool host, const char *rulesKey,
                            const char *rules, S_CampaignNetConfig &config)
{
    HCRYPTPROV provider = 0;
    if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return false;

    const std::string path = std::string("missions/") + missionName + "/";
    auto &content = config.m_Hello.m_Content;
    content.m_CampaignContent = DigestCategory(provider, 1, {{"GameData/Gamedata00.gdt", {}}});
    content.m_MissionContent = DigestCategory(provider, 2, {
        {path + "scene.4ds", {}}, {path + "scene2.bin", {}}, {path + "tree.klz", {}},
        {path + "actors.bin", {}}, {path + "check2.bin", {}}, {path + "sounds.bin", {}},
        {path + "Scripts.dta", {}}, {"Scripts.dta", {}}
    });
    content.m_SoldierContent = DigestCategory(provider, 3, {{"tables/HumanStore.def", {}}});
    content.m_ItemContent = DigestCategory(provider, 4, {
        {"Tables/items.sav", {}}, {"GameData/presets.def", {}}, {"GameData/miss_itm.def", {}}
    });
    content.m_Rules = DigestCategory(provider, 5, {{rulesKey, rules}});

    std::uint64_t randomSession = 0;
    const bool session = !host ||
        (CryptGenRandom(provider, sizeof(randomSession), reinterpret_cast<BYTE *>(&randomSession)) && randomSession != 0);
    CryptReleaseContext(provider, 0);
    if (host) {
        config.m_Session = SessionId(randomSession);
        config.m_Host = PlayerId(1);
    }
    return session && !content.m_CampaignContent.empty() && !content.m_MissionContent.empty() &&
           !content.m_SoldierContent.empty() && !content.m_ItemContent.empty() && !content.m_Rules.empty();
}
