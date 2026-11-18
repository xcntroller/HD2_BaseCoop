# COOP-001 build and runtime baseline

The existing hand port builds and runs on this Fedora workstation through MSVC
under Wine. This task adds build isolation and portable toolchain selection; it
changes no game or engine C++ behavior and introduces no campaign co-op mode.
The [task record](tasks/COOP-001.md) records review/integration status.

## Tested environment

Session date: 2026-11-18 Europe/Prague. Worktree:
`/home/control/Documents/Projects/HD2SS/HD2_BaseCoop/.worktrees/COOP-001`.

| Prerequisite | Version / source used |
| --- | --- |
| OS | Fedora 44 x86_64, kernel 7.3.3-200.fc44 |
| Native Python / GCC | Python 3.14.10; GCC 16.2.1 (20261029) |
| Windows Python | CPython 3.13.9 embedded amd64 from python.org |
| Wine | 11.21 Staging, private prefixes under this worktree's ignored `build/` |
| Target compiler | MSVC 19.44.35230 **for x86**, Hostx64/x86; toolset directory 14.44.35207 |
| Windows SDK | 10.0.26100.0, including x86 UCRT/UM libraries |
| Toolchain download | [msvc-wine](https://github.com/mstorsjo/msvc-wine), commit `5dd115047b7ddcbb53cbc8b5b14163a20f4334fe`; Microsoft release manifest 17.14.45 |
| Import library tool | Fedora `x86_64-w64-mingw32-dlltool` 2.46.0.20260210, with **both** `-m i386 -f --32` |
| MSI extraction | msitools/libmsi1 0.106.58; libgcab1 1.6, extracted locally from Fedora RPMs |
| Test display | Xvfb 21.1.26, displays :99 and :100, each 1280×960×24; game client area 1024×768 |
| Game data | Local Steam app 703320, build 2147249; main menu reports Hidden and Dangerous 2 **1.12.0** |

Game installation: `~/.local/share/Steam/steamapps/common/Hidden and Dangerous 2`.
Tests used independent copies under `build/runtime/{solo,host,guest}`. The installed
archives, original binaries, settings, profiles and Proton prefix were not changed.
Loose `Maps/` files observed here are generated texture/cache files; no loose mission
directory was used to replace the tested Tutorial or native `Co_Sicily2` mission.

## Windows build route

Use Windows Python, Visual Studio C++ x86 tools and a Windows SDK. The engine also
needs MinGW's 32-bit `dlltool.exe` (`DLLTOOL`, PATH or the existing script's common
locations). Tool discovery still uses `vswhere` by default. An explicit environment
setup file can be selected without editing machine-global settings:

```bat
set "HD2_VCVARS32=C:\toolchain\vcvars32.bat"
python _re\tools\build_isolated.py ls3df
python _re\tools\build_isolated.py rw-data
python _re\tools\build_isolated.py game --all --copy C:\HD2Run
```

`HD2_VCVARS32` may also name a Hostx64-to-x86 environment batch. It must configure
the x86 compiler, INCLUDE, LIB, PATH and WindowsSdkDir; selecting an x64 target is
not valid. An explicitly configured nonexistent file is an error, not silent fallback.
The override is consumed by `build_check.py` (also game/DTA builds) and `ls3df/build.py`.

Copy `ls3df/build/msvc/LS3DF.dll` and `rw_data/build/msvc/rw_data.dll` into the same
**copied** game directory as the executable. Keep the installed IJoy, Bink, Ogg and
Vorbis dependencies there. The original LS3DF.dll is incompatible with the port.
No executable, proprietary DLL, archive, toolchain or profile is committed/supplied.

The runnable game retains `/arch:IA32 /O2 /MT /Oy- /GS- /Z7` and `/MACHINE:X86`;
its arithmetic needs x87. `rw_data` also retains `/arch:IA32`. LS3DF retains its
existing x86 MSVC build flags. Do not substitute the syntax checker or an x64/SSE2
game build for this runnable build. Use `--all` after game header changes.

## Fedora MSVC/Wine setup used here

All commands below run from the task worktree root. Downloads/builds stay in
ignored `build/`. Required existing native tools: Python, Git, curl, unzip,
rpm2cpio, cpio, Wine, and the MinGW dlltool above. No global package installation,
Wine prefix, shell environment, HOME or CODEX_HOME change is needed.

Download and extract the MSI tools locally (only x86_64 packages are needed):

```bash
mkdir -p build/tooling/rpms
dnf download --destdir build/tooling/rpms msitools.x86_64 libmsi1.x86_64 libgcab1.x86_64 xorg-x11-server-Xvfb.x86_64
python - <<'PY'
from pathlib import Path
import subprocess
dest = Path('build/tooling/msitools').resolve()
dest.mkdir(exist_ok=True)
for rpm in Path('build/tooling/rpms').glob('*.x86_64.rpm'):
    with subprocess.Popen(['rpm2cpio', str(rpm.resolve())], stdout=subprocess.PIPE) as unpack:
        subprocess.run(['cpio', '-idm', '--quiet'], stdin=unpack.stdout, cwd=dest, check=True)
        unpack.stdout.close()
        assert unpack.wait() == 0
PY
```

The Xvfb binary is consequently `build/tooling/msitools/usr/bin/Xvfb` in this
reproduction. The session extracted its Xvfb RPM separately into `build/tooling/xvfb/`;
that changes only the launch path. Existing Fedora shared-library dependencies are
also prerequisites (`ldd .../Xvfb` and `.../msiextract` must have no missing libraries).

Fetch the downloader at its tested source commit and the Windows toolchain:

```bash
git clone https://github.com/mstorsjo/msvc-wine.git build/tooling/msvc-wine
git -C build/tooling/msvc-wine checkout 5dd115047b7ddcbb53cbc8b5b14163a20f4334fe
(
  cd build/tooling
  PATH="$PWD/msitools/usr/bin:$PATH" LD_LIBRARY_PATH="$PWD/msitools/usr/lib64" \
    python msvc-wine/vsdownload.py --accept-license --major 17 --architecture x86 \
    --host-arch x64 --with-msbuild no --with-asan no --with-dia no --with-atl no \
    --with-devcmd yes --save-manifest --cache msvc-cache --dest msvc
)
mkdir -p build/tooling/python
curl --fail --location --output build/tooling/python-3.13.9-embed-amd64.zip \
  https://www.python.org/ftp/python/3.13.9/python-3.13.9-embed-amd64.zip
unzip -q build/tooling/python-3.13.9-embed-amd64.zip -d build/tooling/python
python _re/tools/configure_wine_msvc.py --msvc-version 14.44.35207 --sdk-version 10.0.26100.0
```

The Microsoft channel can advance; verify the resulting versions against the
table. The saved `17.14.45.manifest` and payload cache retained locally allow the
same selection to be reused with the downloader's `--manifest` option. The session's
manifest is `build/tooling/17.14.45.manifest`. No nonredistributable tools are in Git.

`configure_wine_msvc.py` validates the selected directories and creates three local
files: `vcvars-x86.bat`, `run-python.bat`, and `run_script.py`. It does not install
anything. It uses Wine's default `Z:` mapping to the Unix root. The generated
launcher adds the selected script's directory explicitly, so embedded Python works
without editing its `python313._pth`. Paths containing batch-expansion `%`, quotes
or newlines are rejected. Standard spaces, including `Windows Kits`, are supported.

Start a private test display and define a process-local Windows Python command:

```bash
build/tooling/msitools/usr/bin/Xvfb :99 -screen 0 1280x960x24 -ac -nolisten tcp > build/tooling/xvfb.log 2>&1 &
wine_py() {
  env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all \
    wine64 cmd /c 'build\tooling\run-python.bat' "$@"
}
```

Do not start another server on an occupied display. In this session :99 already
exists; reuse it while its owner is running. These prefixes/displays are test
resources, not the user's desktop or installed game's Proton prefix.

Prepare the four import libraries **before** starting the engine build, serialized
within this worktree:

```bash
python - <<'PY'
from pathlib import Path
import subprocess
root = Path.cwd()
out = root / 'build/isolated/ls3df/default/temp/ls3df_build'
out.mkdir(parents=True, exist_ok=True)
for name in ['binkw32', 'd3d8', 'rw_data', 'vorbisfile']:
    subprocess.run(['x86_64-w64-mingw32-dlltool', '-m', 'i386', '-f', '--32',
                    '-d', str(root / 'ls3df/lib' / (name + '.def')), '-l', name + '.lib'],
                   cwd=out, check=True)
PY
wine_py '_re\tools\build_isolated.py' ls3df
wine_py '_re\tools\build_isolated.py' rw-data
wine_py '_re\tools\build_isolated.py' game --all --copy 'build\runtime\solo'
```

Create the runtime copy before using `--copy`; it must contain the installed data.
Then copy both newly built DLLs into it. After an engine import `.def` change,
regenerate these import libraries before building, or supply a Windows dlltool.
The engine checks their timestamps. The native x64 assembler otherwise emits
64-bit import headers even with `-m i386`; this caused the first link failures.
`-f --32` corrected them; changing only library output names did not fix the issue.

The source's optional icon path is specific to the upstream author's machine.
This build has no icon/title resources and prints that diagnostic; it still loads
the menu and missions. The `ERROR LOADING RESOURCES !!!!` log line was observed at
startup, without preventing the tested flows. No game resources were added to Git.

## Build output isolation and evidence

The wrapper sets TEMP, TMP and TMPDIR **only for the child process**. Each tool's
legacy fixed directory now lives below the current worktree's
`build/isolated/<family>/<variant>/temp`. It holds an exclusive `build.lock` per
family, including engine release/debug variants that share a final DLL destination.
`check` and `link-check` share both objects and lock. Game variants retain their
legacy separate `hd2_game[_cov][_checked]` folders within that root.

| Build | Actual default output in this task worktree |
| --- | --- |
| Game | `build/isolated/game/default/temp/hd2_game/HD2_Port.exe`, PDB and map |
| LS3DF intermediates | `build/isolated/ls3df/default/temp/ls3df_build/` |
| LS3DF final DLL/import library | `ls3df/build/msvc/` |
| rw_data intermediates | `build/isolated/rw-data/default/temp/rw_data_build/` |
| rw_data final DLL/import library | `rw_data/build/msvc/` |
| MSVC diagnostic objects | `build/isolated/check/default/temp/hd2_build_check/` |

A second same-family build in one worktree fails before touching the build files.
An interrupted wrapper preserves its lock because compiler descendants may remain.
Inspect `build.lock/owner.json` and confirm the recorded build and descendants have
stopped before removing a stale lock. Never delete another worker's lock. A normal
child failure propagates its exit code and releases the lock. Direct legacy script
invocations still use fixed system-temp names; use the wrapper consistently.

These checks were actually run:

```bash
python _re/tools/test_build_isolated.py -v
wine_py '_re\tools\test_build_isolated.py' -v
```

Both passed three process-level tests: two concurrent fake worktrees write/read
the same object filename without crossing roots; a second engine variant in one
tree is rejected; a failing child preserves exit 7 and releases the lock. Paths
with spaces are included. These are isolation tests, not simulated game evidence.

Additionally, a detached `.worktrees/COOP-001-cache-probe` at `ca28d52c` and the task
tree ran real MSVC checks of `src/common/c_str.cpp` concurrently through the wrapper.
Each reported `1 files, 0 errors, 0 warnings`, and its printed temporary root and
`files.rsp` named only its own worktree. The invocation in each tree was:

```bat
<COOP-001>\build\tooling\run-python.bat _re\tools\build_isolated.py check src\common\c_str.cpp
```

Both invocations used Wine/Windows Python as above; the probe referenced the task's
launcher by its absolute `Z:` path. Logs: `build/tooling/check-main.log` in COOP-001
and `build/check-probe.log` in the probe. Neither tree consumed `/tmp/hd2_build_check`
or the other's response/object files. The probe has no independent implementation.

The legacy `link_check.py` still expects sibling import-library locations and writes
its diagnostic report under `_re/`; it was not used as runnable-build evidence.
The actual game build compiled and linked the complete executable successfully.
The wrapper's `ls3drf` route isolates that legacy engine's temp output, but its
build/runtime was not exercised here and its tool discovery remains unchanged.

## Runtime preparation and launch

The test prefixes need the user's existing legitimate game registry settings/key,
imported privately from their installed-game prefix. Do not print or commit those
values. Only the game's registry subtree was copied; no account or unrelated
registry state was imported. The test-only `LS3D_setup` blob used 1024×768,
windowed mode, sound disabled and wait-while-inactive disabled. The original
installation and Proton prefix retain their settings.

This reproduces the local copy/registry preparation without displaying the key.
Run it once in a fresh worktree after building (or move the game-directory copy
before the game's `--copy` step). Existing destination directories fail safely:

```bash
python - <<'PY'
from pathlib import Path
import re, shutil, struct
game = Path.home() / '.local/share/Steam/steamapps/common/Hidden and Dangerous 2'
prefix = Path.home() / '.local/share/Steam/steamapps/compatdata/703320/pfx'
root = Path.cwd()
for role in ['solo', 'host', 'guest']:
    dest = root / 'build/runtime' / role
    shutil.copytree(game, dest)
    for source in [root / 'build/isolated/game/default/temp/hd2_game/HD2_Port.exe',
                   root / 'build/isolated/game/default/temp/hd2_game/HD2_Port.pdb',
                   root / 'ls3df/build/msvc/LS3DF.dll', root / 'rw_data/build/msvc/rw_data.dll']:
        shutil.copy2(source, dest / source.name)
    shutil.copy2(prefix / 'drive_c/windows/syswow64/d3d8.dll', dest / 'd3d8.dll')
registry = (prefix / 'system.reg').read_text(errors='replace')
key = r'[Software\\Wow6432Node\\Illusion Softworks\\Hidden & Dangerous 2]'
start = registry.index(key)
end = registry.find('\n[', start + 1)
section = registry[start:end if end >= 0 else None].split('\n', 1)[1]
section = '\n'.join(line for line in section.splitlines() if not line.startswith('#'))
match = re.search(r'"LS3D_setup"=hex:([0-9a-fA-F,\\\s]+)', section)
blob = bytearray.fromhex(re.sub(r'[,\\\s]', '', match.group(1)))
assert len(blob) == 0x23 and blob[0] == 0x0c
struct.pack_into('<II', blob, 2, 1024, 768)
blob[0x15], blob[0x1d], blob[0x1f] = 0, 1, 0
section = section[:match.start()] + '"LS3D_setup"=hex:' + ','.join(f'{b:02x}' for b in blob) + '\n' + section[match.end():]
(root / 'build/runtime/game-settings.reg').write_text(
    'Windows Registry Editor Version 5.00\n\n'
    '[HKEY_LOCAL_MACHINE\\Software\\Wow6432Node\\Illusion Softworks\\Hidden & Dangerous 2]\n' + section + '\n')
PY
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all \
  wine64 regedit /S 'build\runtime\game-settings.reg'
env -u WAYLAND_DISPLAY DISPLAY=:100 WINEPREFIX="$PWD/build/wine-guest" WINEDEBUG=-all \
  wine64 regedit /S 'build\runtime\game-settings.reg'
```

Both X displays must already exist for these commands; Wine initializes a new
guest prefix on first use. `game-settings.reg` contains private installed-game
values and stays ignored. For other installations use their actual data/prefix
locations; do not invent or publish a replacement game key.

The clean system Wine prefix lacked usable d3d8 and native DirectPlay server
support. The already installed game prefix at
`~/.local/share/Steam/steamapps/compatdata/703320/pfx/drive_c/windows/syswow64`
supplied `d3d8.dll` (copied into the runtime directories) and native `dpnet.dll`,
`dpnhpast.dll`, `dpnhupnp.dll`, `dplayx.dll`, `dplaysvr.exe`, `dpnsvr.exe` when present
(copied into the **private test prefixes'** syswow64 directories). No DLL download
or game-code workaround was needed. DirectPlay overrides below select them.

After prefix initialization, copy those existing DirectPlay dependencies:

```bash
python - <<'PY'
from pathlib import Path
import shutil
source = Path.home() / '.local/share/Steam/steamapps/compatdata/703320/pfx/drive_c/windows/syswow64'
for prefix in ['build/wine-prefix', 'build/wine-guest']:
    dest = Path(prefix) / 'drive_c/windows/syswow64'
    for name in ['dpnet.dll', 'dpnhpast.dll', 'dpnhupnp.dll', 'dplayx.dll', 'dplaysvr.exe', 'dpnsvr.exe']:
        if (source / name).is_file():
            shutil.copy2(source / name, dest / name)
PY
```

Native DirectPlay initially chose an IPv6 device. The following command in each
private prefix selected AF_INET and allowed the loopback test:

```bash
env -u WAYLAND_DISPLAY DISPLAY=:99 WINEPREFIX="$PWD/build/wine-prefix" WINEDEBUG=-all \
  wine64 reg add 'HKLM\Software\Wow6432Node\Microsoft\DirectPlay8\IPAddressFamilySettings' \
  /v DefaultIPAddressFamily /t REG_DWORD /d 2 /f
env -u WAYLAND_DISPLAY DISPLAY=:100 WINEPREFIX="$PWD/build/wine-guest" WINEDEBUG=-all \
  wine64 reg add 'HKLM\Software\Wow6432Node\Microsoft\DirectPlay8\IPAddressFamilySettings' \
  /v DefaultIPAddressFamily /t REG_DWORD /d 2 /f
```

Run solo on :99:

```bash
env -u WAYLAND_DISPLAY DISPLAY=:99 LIBGL_ALWAYS_SOFTWARE=1 WINEPREFIX="$PWD/build/wine-prefix" \
  WINEDLLOVERRIDES='d3d8=n,b' WINEDEBUG=-all \
  wine64 build/runtime/solo/HD2_Port.exe -window
```

Exit that instance before hosting. The executable has a single-instance mutex;
two clients on one workstation need separate Wine prefixes (or separate Windows
machines). Start a second Xvfb on :100 with the same screen arguments and initialize
the guest prefix/settings/dependencies as above. Then use these **tested** commands:

```bash
env -u WAYLAND_DISPLAY DISPLAY=:99 LIBGL_ALWAYS_SOFTWARE=1 WINEPREFIX="$PWD/build/wine-prefix" \
  WINEDLLOVERRIDES='d3d8,dpnet,dpnhpast,dpnhupnp=n,b' WINEDEBUG=-all \
  wine64 build/runtime/host/HD2_Port.exe -window -cmd port 11001 domain local \
  style cooperative mapname Sicily2 dedicated 0 sessionname COOP001Baseline \
  maxclients 4 coopdifficulty 2 cooplives 3 voicechat none server
env -u WAYLAND_DISPLAY DISPLAY=:100 LIBGL_ALWAYS_SOFTWARE=1 WINEPREFIX="$PWD/build/wine-guest" \
  WINEDLLOVERRIDES='d3d8,dpnet,dpnhpast,dpnhupnp=n,b' WINEDEBUG=-all \
  wine64 build/runtime/guest/HD2_Port.exe -window -cmd port 11001 domain local \
  voicechat none join 127.0.0.1
```

Order matters: `port` changes the legacy domain to internet, so `domain local` must
follow it. LAN uses fixed DirectPlay port **31567** (`0x7B4F`) and game transport
**31568**, despite the stored `port 11001` setting. `ss -lunp` confirmed IPv4 bindings.
The earlier internet-domain attempt reached CD-key validation failure; it is not
counted as a successful session. These commands test LAN loopback, not internet
listing, NAT traversal, a remote machine, or a dedicated server.

## Actual runtime record and bounded results

Game/engine source tested: task checkpoint `ca28d52c`, based on `7e033add`; gameplay
C++ is unchanged from that base. Final wrapper/configuration tests also ran at
`23079565`. Engine build: 78 translation units compiled, one existing C4127 warning;
link succeeded. Game build: 272 translation units, **zero diagnostics**, link return
0, large-address-aware yes. rw_data compiled/linked successfully. All three PE
machine headers are `0x014c` (i386).

| Tested artifact | SHA-256 |
| --- | --- |
| HD2_Port.exe (4,640,768 bytes) | `fa176b7c610b74538ca856b120bed33d7d8ab5646116b42e43b078be6a5c928d` |
| LS3DF.dll (1,376,768 bytes) | `9d62067139b771034780a44405900d88bcbb2f76647dacddb414160c74f88f9d` |
| rw_data.dll (829,952 bytes) | `8d40a304f668befd72e556f6de031de819e0f03d489470a6af9ec3adf9f14291` |

Solo actions actually performed: main menu → Single Player → H&D2 Original
Campaign → Automatic Selection → Continue → first active soldier → Yes. Tutorial
loaded, its scripted training introduction ran, Space entered the scene, and Resume
allowed movement. Held W for two seconds; the view/position changed. I opened the
inventory with Robert Czakowski's campaign gear and skills. Escape → Exit to Main
Menu → Yes returned normally; End → Yes exited. This is a partial training run,
**not** Tutorial completion, another base mission completion, or save/load evidence.

Native co-op actions actually performed on **Sicily2 / Co_Sicily2**:

1. Host and guest connected through the commands above. Both setup screens listed
   `aaa` and the automatically disambiguated `aaa2` as preparing. Host selected
   Douglas Lauer, guest Larry Smith; no player actor was fabricated by test code.
2. Each selected British green camouflage Battledress from Uniforms, Colt 1911
   from Light Weapons, and its magazine. Accept spawned both players normally.
3. Held host W for three seconds: guest visibly saw the host run ahead. Held guest
   W for six seconds: host visibly saw the guest at the wall to the right. Both
   clients continued rendering the same mission. Inventory displayed committed
   uniform/pistol/magazine and the expected soldier stats on each instance.
4. Weapon selector Z, Z, Space (this profile's WASD bindings) moved the Colt from
   pouches to hands on both instances; HUD and first-person weapon showed 7/1.
   Synthetic primary-fire and reload input was attempted, but no ammunition change
   was established. **Firing/reload are not passed checks**; this observation does
   not establish whether input automation or game behavior caused it. Keep them
   explicit checks in the later controller/mission gates.
5. Holding comma showed the native cooperative scores: Sicily2, both players,
   zero kills/deaths, host name COOP001Baseline, guest ping 6, mission time 00:17:56.
   The original objectives were listed as not completed. No mission victory or
   campaign persistence is claimed.

The native MP equipment/score screens above are existing baseline behavior. They
are not adopted as the future campaign preparation UI or campaign rules.

Local evidence is preserved under ignored `build/evidence/` and `build/tooling/`:
`solo-walk.png`, `solo-inventory.png`, `host-lan-players.png`,
`guest-lan-join.png`, `host-armed-loadout.png`, `guest-armed-loadout.png`,
`guest-observes-host.png`, `host-observes-guest.png`, `host-world-inventory.png`,
`guest-inventory-native.png`, `host-pistol-ready.png`, `guest-pistol-ready.png`,
`host-two-player-status.png`; compiler logs `game-build.log`, `ls3df-build.log`,
`rw-data-build.log`, `wine-isolation-tests.log`, `check-main.log`. Runtime stdout
and game logs are in each runtime copy. These local files contain game imagery
or machine paths, so they are not shipped as source. Re-run the steps to reproduce
on another machine. No `.RPT` crash report was generated during these runs. Both test clients were subsequently closed; host used Exit to Main Menu confirmation and normal End/Yes exit. The private X displays/toolchain remain available to other task workers; do not terminate them while another worker is using them.

Initial failures are retained in the handoff rather than counted as passes:
native Linux Python cannot execute the legacy `.bat`; GCC's `-m32` fallback lacks
`gnu/stubs-32.h` on this machine; uncorrected import libraries failed the LS3DF link;
system Wine DirectPlay could not host; IPv6-only binding and the wrong command
order prevented the initial joins. The tested MSVC/Wine/private-prefix route
resolved the build, rendering, hosting and connection prerequisites. The GCC
fallback remains supplementary syntax tooling and is not a runnable build.

## Reviewer repeat

Recreate the toolchain and private runtime copies, build through the wrapper, and
verify the x86 PE headers and x87 game flags. Repeat both process-test commands and
the two-worktree MSVC check. Repeat the solo Tutorial route and two-instance native
co-op movement/inventory flow above, recording any differences. Expect the same
campaign menus/soldier identity, successful mission loading, two connected native
players and visible movement replication. Verify firing/reload separately; they
were not established by this baseline. Full campaign missions, healing, vehicle
interactions, save/load and future campaign co-op require their own runtime gates.
