#if !defined(_DEBUG)
#error Progression regressions require a checked build in a disposable worktree.
#endif

bool StartSoloRegressionPreparation(const std::function<bool(C_CampaignNode *)> &start)
{
    g_playMode->mode = PLAY_MODE_CAMPAIGN;
    g_playMode->singleMission = 0;
    g_campaign->SetCurrentIndex(0);
    if (!g_campaign->AdvanceToMission())
        return false;
    while (auto *step = g_campaign->NextStep()) {
        if (g_campaign->GetMissionNumber(0) != 1 || step->GetType() != CAMPAIGN_NODE_SCREEN ||
            static_cast<C_CampaignScreen *>(step)->GetKind() != C_CampaignScreen::KIND_EQUIPMENT)
            continue;
        g_missionSquad->Clear();
        for (const int index : GetInitialCampaignRoster()) {
            auto *soldier = g_missionSquad->AddSoldier();
            if (!soldier->SetProfile(index, 0))
                return false;
            soldier->autoEquip = true;
        }
        return start(step);
    }
    return false;
}

void TickPairedProgressionProbe(const char *scenario)
{
    if (!g_menuSession || !g_step)
        return;
    const bool leaveReport = std::strcmp(scenario, "report-departure") == 0;
    const bool leaveLoading = std::strcmp(scenario, "loading-departure") == 0;
    if (std::strcmp(scenario, "campaign") != 0 && !leaveReport && !leaveLoading)
        return;

    static std::optional<S_CampaignControlToken> previousInput;
    static bool checkedPreviousInput = false;
    if (!g_guest && g_ownership) {
        const auto phase = g_ownership->GetSnapshot()->m_Context;
        if (!previousInput && phase.m_MissionGeneration.Value() == 1 && phase.m_Phase == Phase::Running)
            previousInput = g_ownership->GetControlToken(kHostPlayer);
        if (previousInput && !checkedPreviousInput && previousInput->m_Generation < phase.m_MissionGeneration) {
            checkedPreviousInput = true;
            const auto reason = g_ownership->ValidateInput(kHostPlayer, *previousInput);
            LogDevelopmentCampaign("progression fixture checked previous input token",
                                   static_cast<unsigned>(reason), phase.m_MissionGeneration.Value());
        }
    }

    static std::optional<Wire::S_ReportKey> report;
    static Clock::time_point opened;
    static bool earlyAdvance = false;
    if (const auto *view = GetLiveCampaignReport()) {
        if (!report || !(view->m_Report == *report)) {
            report = view->m_Report;
            opened = Clock::now();
            earlyAdvance = false;
        }
        const auto age = Clock::now() - opened;
        if (leaveReport && g_guest && age >= std::chrono::seconds(2)) {
            LogDevelopmentCampaign("progression fixture leaving the waiting report");
            g_netGame.Close(true);
            return;
        }
        if (age >= std::chrono::seconds(g_guest ? 4 : 1))
            ReadyLiveCampaignReport();
        if (!g_guest && !earlyAdvance && age >= std::chrono::seconds(2)) {
            earlyAdvance = true;
            LogDevelopmentCampaign("progression fixture attempted early advancement");
            AdvanceLiveCampaignReport();
        }
        if (!g_guest && age >= std::chrono::seconds(6) && CanAdvanceLiveCampaignReport()) {
            AdvanceLiveCampaignReport();
            AdvanceLiveCampaignReport();
        }
        return;
    }

    if (leaveLoading && g_guest && g_guest->m_Manifest &&
        g_guest->m_Manifest->m_Context.m_MissionGeneration.Value() > 1) {
        LogDevelopmentCampaign("progression fixture leaving the loading barrier");
        g_netGame.Close(true);
        return;
    }
    if (!g_continuedPreparation)
        return;
    auto *model = g_menuSession->GetPreparation();
    const auto state = model->GetSnapshot().m_Snapshot;
    if (!state || state->m_Context.m_Phase != Phase::Preparation || !state->m_Context.m_LocalPlayer)
        return;
    const auto local = *state->m_Context.m_LocalPlayer;
    const auto participant = std::find_if(state->m_Participants.begin(), state->m_Participants.end(),
        [&](const auto &entry) { return entry.m_Player == local; });
    if (participant == state->m_Participants.end())
        return;
    static PreparationGeneration readied;
    static PreparationGeneration deployed;
    if (participant->m_ReadyAt != state->m_PreparationRevision && readied != state->m_Generation) {
        const S_CampaignPreparationRequestContext request{
            {state->m_Context.m_Session, local, model->NextRequestId()}, state->m_Generation,
            state->m_PreparationRevision};
        model->SetReady({request, true});
        readied = state->m_Generation;
    }
    if (!g_guest && state->m_CanDeploy.m_Allowed && deployed != state->m_Generation) {
        const S_CampaignPreparationRequestContext request{
            {state->m_Context.m_Session, local, model->NextRequestId()}, state->m_Generation,
            state->m_PreparationRevision};
        model->RequestDeploy({request});
        deployed = state->m_Generation;
    }
}

void TickProgressionRegression(C_GameApp &app, const std::function<bool(C_CampaignNode *)> &start)
{
    const char *scenario = std::getenv("HD2_PROGRESSION_DEBUG");
    if (!scenario)
        return;
    if (IsCampaignNet()) {
        TickPairedProgressionProbe(scenario);
        return;
    }

    const bool solo = (g_gameRole & GAME_ROLE_LOCAL) == GAME_ROLE_LOCAL;
    const bool server = (g_gameRole & GAME_ROLE_SERVER) != 0;
    if (solo && std::strcmp(scenario, "solo") == 0) {
        static bool loaded = false;
        static bool finished = false;
        if (!loaded && app.m_state->GetTypeId() == GAME_STATE_TYPE_MAIN_MENU) {
            if (const char *save = std::getenv("HD2_PROGRESSION_SOLO_SAVE")) {
                app.RequestLoad(C_str(save));
                loaded = true;
            } else {
                loaded = StartSoloRegressionPreparation(start);
            }
        }
        auto *game = GetGame();
        if (loaded && !finished && game && game->GetMission() &&
            std::strcmp(game->GetMission()->GetName(), "Arctic1") == 0 &&
            game->GetMissionTime() > 5000 && game->IsPlayerInputEnabled() && !g_scriptModule.IsCutsceneRunning()) {
            finished = true;
            // Stop mission scripts so they cannot change the seeded objectives
            // during the original end-of-mission fade.
            g_scriptModule.StopMission(true);
            game->SetObjectiveStatus(4, OBJECTIVE_OPEN);
            for (int objective = 1; objective <= 4; ++objective)
                game->SetObjectiveStatus(objective, OBJECTIVE_COMPLETED);
            LogDevelopmentCampaign("solo regression seeded original mission objectives");
            game->EndMission(100);
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
