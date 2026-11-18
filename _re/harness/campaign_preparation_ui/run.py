"""Build/run standalone behavior tests or build the native LS3D preview (MSVC x86)."""
import argparse
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler', choices=['gcc', 'msvc'], required=True)
    p.add_argument('--native', action='store_true')
    p.add_argument('--vcvars', default=os.environ.get('HD2_VCVARS32'))
    p.add_argument('--engine-lib', type=Path)
    p.add_argument('--data-lib', type=Path)
    a = p.parse_args()
    output_root = ROOT / 'build/campaign_preparation_ui'
    output_root.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=a.compiler+'-', dir=output_root))
    print('Output:', output, flush=True)
    sources = [ROOT/'src/game/campaign_preparation_screen.cpp', HERE/('native.cpp' if a.native else 'behavior.cpp')]
    if a.compiler == 'gcc':
        if a.native: p.error('Native LS3D preview requires MSVC x86')
        command = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic', '-Isrc', *map(str,sources), '-o', str(output/'behavior')]
    else:
        if os.name != 'nt' or not a.vcvars: p.error('Run Windows Python with --vcvars or HD2_VCVARS32')
        command = ['cl', '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', '/permissive-', '/Gy', '/MT', '/O2', '/arch:IA32', '/DNOMINMAX', '/DWIN32_LEAN_AND_MEAN', '/D_CRT_SECURE_NO_WARNINGS', '/Isrc', *map(str,sources)]
        if a.native:
            if not a.engine_lib or not a.data_lib: p.error('Preview requires --engine-lib and --data-lib')
            # Reuse the port's texture-font methods, excluding its global profile-bound manager.
            # Generated source remains in ignored build output; no legacy file is edited.
            font_source = (ROOT/'src/game/font.cpp').read_text()
            marker = '// --- the font manager (part 9a-i)'
            if font_source.count(marker) != 1: raise RuntimeError('font.cpp extraction boundary changed')
            font_renderer = output/'font_renderer.cpp'
            font_renderer.write_text(font_source.split(marker)[0])
            command += [str(font_renderer), str(ROOT/'src/common/c_str.cpp')]
        command += ['/Fe'+str(output/('preview.exe' if a.native else 'behavior.exe')), '/Fo'+str(output)+'\\', '/link', '/MACHINE:X86', '/OPT:REF', '/OPT:ICF']
        if a.native: command += [str(a.engine_lib), str(a.data_lib), 'user32.lib', 'gdi32.lib']
    (output/'command.txt').write_text(subprocess.list2cmdline(command)+'\n')
    actual = command
    if a.compiler == 'msvc':
        if any(c in str(a.vcvars) for c in '\r\n"%'): p.error('Unsupported vcvars path')
        batch=output/'build.bat'
        batch.write_text('@echo off\ncall "'+a.vcvars+'"\nif errorlevel 1 exit /b %errorlevel%\n'+subprocess.list2cmdline(command)+'\nexit /b %errorlevel%\n')
        actual=['cmd','/d','/c',str(batch)]
    r=subprocess.run(actual,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,errors='replace')
    (output/'compile.log').write_text(r.stdout)
    print(r.stdout,flush=True)
    if r.returncode: raise SystemExit(r.returncode)
    if not a.native:
        r=subprocess.run([str(output/('behavior.exe' if a.compiler=='msvc' else 'behavior'))],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        (output/'execute.log').write_text(r.stdout);print(r.stdout)
        if r.returncode: raise SystemExit(r.returncode)
    print('PASS:', 'native preview build' if a.native else 'behavior harness', output)

if __name__=='__main__': main()
