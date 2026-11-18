# Arctic1 controlled host and guest checks

These scenarios run two real game instances. They establish test conditions in
the host and exercise the production authority, inventory, interaction, and
terminal replication paths. The success scenario seeds the original objective
statuses. A pass establishes successful terminal replication in the fixture.
The full campaign route is a separate playtest.

Use the installed Sabre Squadron 1.12 data, a separate runtime copy per peer, and
the pinned x86 build setup in [the build guide](../../../docs/build_baseline.md).

1. Create a disposable checkout with `git worktree add --detach <test-worktree> HEAD`.
   From the task checkout, run
   `python3 _re/harness/campaign_arctic1/install_debug.py <test-worktree>`.
   Build in the disposable checkout with
   `python _re/tools/build_isolated.py game --checked --all`. Use the same
   LS3DF and rw_data import libraries as the production build. The installer
   requires a detached worktree and the fixture requires a checked build.
2. Copy the checked executable into the private host runtime. The guest can
   use the ordinary executable from the same source. Start the host with
   `-campaign-coop host` and the guest with `-campaign-coop join 127.0.0.1`.
   Set `HD2_ARCTIC1_DEBUG` on the host process only. Keep it unset on the guest.
3. Press Escape on the host after both peers have started the aircraft scene.
   Run each scenario in a fresh pair with cleared per-process logs.
   - `wire-success` moves the original cutters to the host belt, puts the
     host beside panel 99, and submits the stock wire-cut action. After the
     native timer, the host breaks the fence and the guest presents the
     break. The fixture then induces one native squad casualty and seeds the
     four successful objective statuses. Both peers must enter Ending with
     the same statuses. The host terminal kit must retain the cutters and
     the result must retain the seeded script save value.
   - `guide-failure` induces native guide death through the same damage
     entry point the script uses. The original script fails the objective.
     The fixture attempts to complete the failed guide objective once and
     verifies that the terminal result still reports critical failure.
4. Preserve both `campaign-dev.log` files and run
   `python3 _re/harness/campaign_arctic1/verify.py <scenario> <host-log> <guest-log>`.

Debug setup only affects the disposable executable. Build the normal executable
from the task checkout for user playtesting.
