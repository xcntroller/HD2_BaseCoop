"""Install the Arctic1 host scenarios in a detached throwaway checkout."""
import argparse
from pathlib import Path
import shutil
import subprocess


def install(root):
    target = root / "src/game/campaign_dev.cpp"
    branch = subprocess.run(
        ["git", "-C", str(root), "symbolic-ref", "--quiet", "HEAD"],
        capture_output=True,
    )
    if branch.returncode != 1 or not (root / ".git").is_file() or not target.is_file():
        raise SystemExit("Use a detached Git worktree created for the checked test build.")
    changed = subprocess.run(
        ["git", "-C", str(root), "diff", "--quiet", "HEAD", "--", "src/game/campaign_dev.cpp"],
        capture_output=True,
    )
    if changed.returncode:
        raise SystemExit("The disposable worktree already has changes to campaign_dev.cpp.")

    source = target.read_text()
    anchors = {
        "#include <cstdio>": "#include <cstdio>\n#include <cstdlib>\n#include \"game/items.h\"",
        "void PublishTerminalMissionUpdate(const S_CampaignMissionResult &result, const S_CampaignOwnershipSnapshot &state)\n{":
            "#include \"host_scenarios.inc\"\n\nvoid PublishTerminalMissionUpdate(const S_CampaignMissionResult &result, const S_CampaignOwnershipSnapshot &state)\n{",
        "        if (g_liveSendFailed || !PublishMissionUpdate(Clock::now(), *g_ownership->GetSnapshot())) {":
            "        TickArctic1HostTest();\n        if (g_liveSendFailed || !PublishMissionUpdate(Clock::now(), *g_ownership->GetSnapshot())) {",
        '    LogDevelopmentCampaign("host captured terminal mission outcome", result.m_Success, result.m_Soldiers.size());':
            '    ObserveArctic1TestResult(result);\n    LogDevelopmentCampaign("host captured terminal mission outcome", result.m_Success, result.m_Soldiers.size());',
    }
    for old, new in anchors.items():
        if source.count(old) != 1:
            raise SystemExit(f"Expected exactly one insertion point for {old!r}.")
        source = source.replace(old, new)
    shutil.copyfile(Path(__file__).with_name("host_scenarios.inc"), target.with_name("host_scenarios.inc"))
    target.write_text(source)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("worktree", type=Path)
    install(parser.parse_args().worktree.resolve())
