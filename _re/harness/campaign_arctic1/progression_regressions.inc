#if !defined(_DEBUG)
#error Progression regressions require a checked build in a disposable worktree.
#endif

void TickProgressionRegression(C_GameApp &app)
{
    if (IsCampaignNet())
        return;
    const char *scenario = std::getenv("HD2_PROGRESSION_DEBUG");
    if (!scenario)
        return;

    const bool solo = (g_gameRole & GAME_ROLE_LOCAL) == GAME_ROLE_LOCAL;
    const bool server = (g_gameRole & GAME_ROLE_SERVER) != 0;
    if (solo && std::strcmp(scenario, "solo") == 0) {
        static bool loaded = false;
        static bool finished = false;
        if (!loaded && app.m_state->GetTypeId() == GAME_STATE_TYPE_MAIN_MENU) {
            if (const char *save = std::getenv("HD2_PROGRESSION_SOLO_SAVE")) {
                app.RequestLoad(C_str(save));
                loaded = true;
            }
        }
        auto *game = GetGame();
        if (loaded && !finished && game && game->GetMission() &&
            std::strcmp(game->GetMission()->GetName(), "Arctic1") == 0 &&
            game->GetMissionTime() > 5000 && game->IsPlayerInputEnabled() && !g_scriptModule.IsCutsceneRunning()) {
            finished = true;
            game->SetObjectiveStatus(4, OBJECTIVE_OPEN);
            for (int objective = 1; objective <= 4; ++objective)
                game->SetObjectiveStatus(objective, OBJECTIVE_COMPLETED);
            LogDevelopmentCampaign("solo regression seeded original mission objectives");
        }
        return;
    }

    if (solo || std::strcmp(scenario, "native-map") != 0)
        return;
    const auto *game = GetGame();
    if (!game || !game->IsMissionLoaded() || !game->GetMission() || game->GetMissionTime() < 1000)
        return;
    const char *map = game->GetMission()->GetName();
    static bool initial = false;
    static bool changed = false;
    if (!initial && _stricmp(map, "Co_Sicily2") == 0) {
        initial = true;
        LogDevelopmentCampaign("native LAN regression loaded Sicily2", server);
    }
    if (!changed && _stricmp(map, "Co_Sicily1") == 0) {
        changed = true;
        LogDevelopmentCampaign("native LAN regression loaded Sicily1", server);
    }
    if (initial && !changed && server && game->GetMissionTime() >= 30000 && g_netGame.GetPlayerCount() > 1) {
        LogDevelopmentCampaign("native LAN regression requested next map");
        ChangeMap(nullptr);
        initial = false;
    }
}
