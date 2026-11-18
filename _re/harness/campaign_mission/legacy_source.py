"""Execute selected checked-out native adapter/legacy bodies with detached services."""
import importlib.util
from pathlib import Path


def generate(root: Path, output: Path):
    spec = importlib.util.spec_from_file_location('preparation_legacy_source', root / '_re/harness/campaign_preparation/legacy_source.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    bodies = []
    for path, signatures in {
        'src/game/campaign_mission_native.cpp': ['E_CampaignReason BuildCampaignNativeSpawns(', 'void HandleCampaignMissionDeparture('],
        'src/game/spawn_point.cpp': ['C_Human *C_SpawnPoint::SpawnCampaign(', 'void C_SpawnPoint::NotifyCampaignSpawned('],
        'src/game/game_rounds.cpp': ['void C_game::OnPlayerLeft(', 'void ChangeMap('],
        'src/game/profile.cpp': ['int C_GameProfile::GetDifficulty('],
        'src/script/script_vm.cpp': ['C_TeamBase *GetPlayerTeamForMode('],
    }.items():
        source = (root / path).read_text()
        bodies.extend(module.extract(source, signature) for signature in signatures)
    (output / 'legacy_bodies.inc').write_text('// Generated from checked-out native source; never copied algorithms.\n' + '\n'.join(bodies))
