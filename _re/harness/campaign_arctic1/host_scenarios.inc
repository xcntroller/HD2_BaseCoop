#if !defined(_DEBUG)
#error Arctic1 test scenarios require a checked build in a disposable worktree.
#endif

bool MoveArctic1TestCutters(C_Human &host)
{
    C_Item *ammo = host.FindBeltItem(ITEM_BEHAVIOUR_AMMO);
    C_Item *cutters = host.FindItem(59);
    return ammo && cutters &&
        RequestDevelopmentCampaignInventoryMove(host.GetNetId(), ammo->GetUid(), host.GetNetId(),
            ITEM_CONTAINER_BELT, ITEM_CONTAINER_BACKPACK_CONTENTS, 0, -1) &&
        RequestDevelopmentCampaignInventoryMove(host.GetNetId(), cutters->GetUid(), host.GetNetId(),
            ITEM_CONTAINER_BACKPACK_CONTENTS, ITEM_CONTAINER_BELT, 0, -1);
}

void ObserveArctic1TestResult(const S_CampaignMissionResult &result)
{
    const char *scenario = std::getenv("HD2_ARCTIC1_DEBUG");
    if (scenario && (std::strcmp(scenario, "wire-success") == 0 ||
                     std::strcmp(scenario, "native-objectives") == 0))
        LogDevelopmentCampaign("arctic1 test captured mission script value", 1, result.m_ScriptValues[1]);
}

void PlaceAtArctic1Entrance(C_Human &human, const S_vector &position)
{
    human.GetFrame()->SetPos(position);
    human.GetFrame()->Update();
    human.SetPosition(position);
}

void TickArctic1NativeObjectives(C_game &game)
{
    enum class Stage { Setup, Entrance, Garrison, Gathering, Done };
    static Stage stage = Stage::Setup;
    static std::uint32_t waitUntil = 0;
    const auto status = [&game](int objective) {
        return game.GetMission()->GetObjective(objective - 1)->status;
    };
    const auto findActor = [&game](const char *name) {
        return GetFrameActorAs<C_Human>(game.GetScene()->FindFrame(name, FRAME_MASK_ALL), ACTOR_HUMAN);
    };
    const auto kill = [](C_Human &human) {
        human.Hit(HIT_SCRIPT_KILL, -1.0f, S_vector(0, 0, 0), S_vector(0, 0, 0), nullptr, 0);
    };
    auto *host = static_cast<C_Human *>(g_actorsModule.GetActorById(0));
    if (!host || !host->IsAlive())
        return;

    // Keep stray fire from disrupting the deliberate waits at the entrance.
    if (stage != Stage::Setup && status(4) != OBJECTIVE_COMPLETED && host->GetHealthPercent() < 50)
        host->SetHealthPercent(100);

    const auto now = game.GetMissionTime();

    switch (stage) {
    case Stage::Setup: {
        if (status(1) != OBJECTIVE_OPEN)
            return;
        stage = Stage::Done;
        auto *objectives = game.GetScene()->FindFrame("objectives", FRAME_MASK_ALL);
        auto *script = objectives && objectives->GetData() ? objectives->GetData()->script : nullptr;
        if (!script || status(2) != OBJECTIVE_OPEN || status(3) != OBJECTIVE_OPEN)
            return;
        const char *const guards[] = {
            "walker_guard_1", "walker_guard_2", "walker_guard_3", "walker_guard_4",
            "Undereground_guard_1", "Undereground_guard_2", "see_sea_1", "Gate_Inquisitor",
            "Gate_Guard", "mole_guard", "roof_walker", "metar", "Kulometcik",
            "Gate_Guard_1", "Gate_Guard_2",
        };
        for (const char *name : guards) {
            auto *guard = findActor(name);
            if (!guard || !guard->IsAlive()) {
                LogDevelopmentCampaign(name);
                return;
            }
            kill(*guard);
        }
        // Supply the guide's completion signal. The remaining objectives use their
        // original predicates for range, garrison deaths and squad gathering.
        script->SendSignal(4);
        PlaceAtArctic1Entrance(*host, S_vector(21.45f, 1.79f, 52.74f));
        LogDevelopmentCampaign("arctic1 test placed host near entrance with one guard alive");
        waitUntil = now + 1500;
        stage = Stage::Entrance;
        return;
    }
    case Stage::Entrance: {
        if (now < waitUntil || status(2) != OBJECTIVE_COMPLETED)
            return;
        stage = Stage::Done;
        auto *lastGuard = findActor("see_sea_2");
        if (!lastGuard || !lastGuard->IsAlive() || status(3) != OBJECTIVE_OPEN ||
            status(4) != OBJECTIVE_INACTIVE)
            return;
        LogDevelopmentCampaign("arctic1 test observed entrance and outstanding guard");
        kill(*lastGuard);
        stage = Stage::Garrison;
        return;
    }
    case Stage::Garrison:
        if (status(3) != OBJECTIVE_COMPLETED || status(4) != OBJECTIVE_OPEN)
            return;
        LogDevelopmentCampaign("arctic1 test observed garrison completion and gathering open");
        // Allow the original completion subtitles and the gathering trigger to run
        // while three squad members are still outside the range.
        waitUntil = now + 10000;
        stage = Stage::Gathering;
        return;
    case Stage::Gathering: {
        if (now < waitUntil)
            return;
        stage = Stage::Done;
        if (status(4) != OBJECTIVE_OPEN)
            return;
        const S_vector positions[] = {{20.69f, 1.77f, 55.96f}, {20.69f, 1.77f, 54.96f},
                                      {21.69f, 1.77f, 55.96f}};
        for (int slot = 1; slot < 4; ++slot) {
            auto *human = static_cast<C_Human *>(g_actorsModule.GetActorById(slot));
            if (!human || !human->IsAlive())
                return;
            PlaceAtArctic1Entrance(*human, positions[slot - 1]);
        }
        LogDevelopmentCampaign("arctic1 test brought remaining squad into native gathering range");
        return;
    }
    case Stage::Done:
        return;
    }
}

void TickArctic1HostTest()
{
    static bool started = false;
    static bool waitingForCutters = false;
    static bool checkedFailure = false;
    static C_WireCut *wire = nullptr;
    auto *game = GetGame();
    if (!game || !game->IsPlayerInputEnabled() || !g_control->IsSynchronized() ||
        g_scriptModule.IsCutsceneRunning())
        return;
    if (wire) {
        if (!wire->IsCut())
            return;
        wire = nullptr;
        // Exercise the native terminal capture and transport with a seeded success.
        // The authored mission scripts and objective definitions still run unchanged.
        auto *casualty = g_actorsModule.GetActorById(3);
        if (casualty && casualty->GetType() == ACTOR_HUMAN)
            casualty->Hit(HIT_SCRIPT_KILL, -1.0f, S_vector(0, 0, 0), S_vector(0, 0, 0), nullptr, 0);
        g_scriptModule.GetSaveGameValues()[1] = 3;
        game->SetObjectiveStatus(4, OBJECTIVE_OPEN);
        for (int objective = 1; objective <= 4; ++objective)
            game->SetObjectiveStatus(objective, OBJECTIVE_COMPLETED);
        LogDevelopmentCampaign("arctic1 test seeded success");
        return;
    }
    const char *scenario = std::getenv("HD2_ARCTIC1_DEBUG");
    if (!scenario)
        return;
    if (std::strcmp(scenario, "native-objectives") == 0) {
        TickArctic1NativeObjectives(*game);
        return;
    }
    if (started && !waitingForCutters) {
        if (!checkedFailure && std::strcmp(scenario, "guide-failure") == 0 && game->IsMissionFailed()) {
            checkedFailure = true;
            game->SetObjectiveStatus(1, OBJECTIVE_COMPLETED);
            const auto status = game->GetMission()->GetObjective(0)->status;
            LogDevelopmentCampaign("arctic1 test tried to complete failed guide", status);
        }
        return;
    }
    if (game->GetMission()->GetObjective(0)->status != OBJECTIVE_OPEN)
        return;
    started = true;
    if (std::strcmp(scenario, "guide-failure") == 0) {
        auto *frame = game->GetScene()->FindFrame("Rebel", FRAME_MASK_ALL);
        auto *guide = GetFrameActorAs<C_Human>(frame, ACTOR_HUMAN);
        if (!guide) {
            LogDevelopmentCampaign("arctic1 test missing guide");
            return;
        }
        LogDevelopmentCampaign("arctic1 test induced native guide death");
        guide->Hit(HIT_SCRIPT_KILL, -1.0f, S_vector(0, 0, 0), S_vector(0, 0, 0), nullptr, 0);
        return;
    }
    if (std::strcmp(scenario, "wire-success") != 0)
        return;
    const auto token = g_ownership->GetControlToken(kHostPlayer);
    auto *host = token ? FindWorldActionHuman(*token) : nullptr;
    auto *target = g_temporaryActors.FindActor(99);
    if (!host || !target || target->GetType() != ACTOR_WIRE_CUT) {
        LogDevelopmentCampaign("arctic1 test invalid wire setup");
        return;
    }
    if (!waitingForCutters) {
        if (!MoveArctic1TestCutters(*host)) {
            LogDevelopmentCampaign("arctic1 test could not equip cutters");
            return;
        }
        waitingForCutters = true;
    }
    if (!host->FindWireCutter())
        return;
    waitingForCutters = false;
    // A position previously reached by native movement in the south approach.
    const S_vector position(-24.16f, 1.87f, 25.74f);
    host->GetFrame()->SetPos(position);
    host->GetFrame()->Update();
    host->SetPosition(position);
    host->SetViewDir({0.451f, 0.0f, 0.892f, 0.0f});
    S_HumanCommand command;
    command.category = HUMAN_COMMAND_VEHICLE;
    command.type = HUMAN_COMMAND_CUT_WIRE;
    command.param1 = target->GetNetId();
    if (!RequestDevelopmentCampaignWorldAction(host->GetNetId(), command)) {
        const auto *frame = target->GetFrame();
        const auto *named = game->GetScene()->FindFrame(frame->GetName(), FRAME_MASK_ALL);
        LogDevelopmentCampaign("arctic1 test wire identity and reach",
            named == frame, InWorldActionReach(*host, *target, 1.8f));
        LogDevelopmentCampaign("arctic1 test wire ownership and tool",
            IsCampaignLocalActor(host->GetNetId()), host->FindWireCutter() != nullptr);
        LogDevelopmentCampaign("arctic1 test wire request failed");
        return;
    }
    wire = static_cast<C_WireCut *>(target);
    LogDevelopmentCampaign("arctic1 test started native wire interaction", command.param1);
}
