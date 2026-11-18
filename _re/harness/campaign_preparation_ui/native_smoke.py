"""Exercise the actual native window with Win32 SendInput and retain screenshot/input evidence.
Run Windows Python from the private runtime directory after building preview.exe.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
from pathlib import Path
import re
import shutil
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--width', type=int, default=1024)
    parser.add_argument('--height', type=int, default=768)
    args = parser.parse_args()
    user = c.windll.user32
    user.GetForegroundWindow.restype = w.HWND
    user.SetForegroundWindow.argtypes = [w.HWND]
    user.ClientToScreen.argtypes = [w.HWND, c.POINTER(w.POINT)]
    user.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
    class MI(c.Structure):
        _fields_ = [('dx', w.LONG), ('dy', w.LONG), ('data', w.DWORD), ('flags', w.DWORD), ('time', w.DWORD), ('extra', c.c_size_t)]
    class KI(c.Structure):
        _fields_ = [('vk', w.WORD), ('scan', w.WORD), ('flags', w.DWORD), ('time', w.DWORD), ('extra', c.c_size_t)]
    class U(c.Union):
        _fields_ = [('mouse', MI), ('key', KI)]
    class Input(c.Structure):
        _fields_ = [('type', w.DWORD), ('data', U)]
    user.SendInput.argtypes = [w.UINT, c.POINTER(Input), c.c_int]
    log = Path(f'native-{args.width}x{args.height}.log')
    stream = log.open('w')
    process = subprocess.Popen([str(Path('preview.exe').resolve()), str(args.width), str(args.height)], stdout=stream, stderr=subprocess.STDOUT)
    evidence = []
    def state():
        text = log.read_text(errors='replace')
        lines = [line for line in text.splitlines() if line.startswith('STATE ')]
        return dict((key, int(value)) for key, value in re.findall(r'(\w+)=(\d+)', lines[-1])) if lines else {}
    def expect(description, **values):
        end = time.monotonic() + 6
        while time.monotonic() < end:
            actual = state()
            if all(actual.get(k) == v for k, v in values.items()):
                evidence.append((description, actual)); print('PASS:', description, actual, flush=True); return
            if process.poll() is not None: raise AssertionError(f'Preview exited {process.returncode}: {log.read_text()}')
            time.sleep(.05)
        raise AssertionError(f'{description}: expected {values}, got {state()}')
    def send(event):
        if user.SendInput(1, c.byref(event), c.sizeof(event)) != 1: raise RuntimeError('SendInput failed')
    def key(vk, shift=False):
        if shift: send(Input(1, U(key=KI(0x10,0,0,0,0))))
        send(Input(1,U(key=KI(vk,0,0,0,0)))); time.sleep(.12)
        send(Input(1,U(key=KI(vk,0,2,0,0))))
        if shift: send(Input(1,U(key=KI(0x10,0,2,0,0))))
        time.sleep(.15)
    try:
        handles = []
        @c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
        def visit(handle, _):
            pid = w.DWORD(); user.GetWindowThreadProcessId(handle, c.byref(pid))
            if pid.value == process.pid and user.IsWindowVisible(handle): handles.append(handle)
            return True
        end=time.monotonic()+20
        while not handles and time.monotonic()<end:
            user.EnumWindows(visit,0); time.sleep(.05)
        if not handles: raise AssertionError('Native window not created')
        window=handles[0]; user.SetForegroundWindow(window); time.sleep(.25)
        if user.GetForegroundWindow()!=window: raise AssertionError('Could not focus native fixture')
        scale=min(args.width/1024,args.height/768)
        left=(args.width-1024*scale)/2; top=(args.height-768*scale)/2
        def click(x,y):
            point=w.POINT(round(left+x*scale),round(top+y*scale)); user.ClientToScreen(window,c.byref(point))
            user.SetCursorPos(point.x,point.y)
            send(Input(0,U(mouse=MI(0,0,0,2,0,0))));time.sleep(.12)
            send(Input(0,U(mouse=MI(0,0,0,4,0,0))));time.sleep(.15)
        def capture(name):
            source=Path(f"coop007-{state()['scenario']}-{args.width}x{args.height}.tga")
            source.unlink(missing_ok=True);key(0x7B)
            end=time.monotonic()+5
            while not source.exists() and time.monotonic()<end: time.sleep(.05)
            if not source.exists(): raise AssertionError('Screenshot not produced')
            target=Path(f'captures-{args.width}x{args.height}');target.mkdir(exist_ok=True)
            shutil.copy2(source,target/(name+'.tga'))
        expect('host initial',selected=1,pending=0,stock=1)
        capture('01-host')
        click(550,140);expect('mouse selects AI without ownership proposal',selected=3,pending=0,stock=1)
        click(650,570);expect('pending edit keeps committed stock',selected=3,pending=1,stock=1)
        capture('02-pending')
        key(0x79);expect('accepted edit updates stock once',pending=0,stock=0)
        click(400,393);click(840,570);expect('return queued',pending=1,stock=0)
        key(0x79);expect('return conserves stock',pending=0,stock=1)
        click(70,140);key(9);key(13);expect('Tab Enter selects another human',selected=2,pending=0)
        click(650,570);expect('host cannot edit another human',selected=2,pending=0,stock=1)
        click(70,140);key(9);key(9,shift=True);key(32);expect('Shift Tab Space activation',selected=1,focus=0,pending=0)
        key(0x71);expect('guest scenario',scenario=2,selected=1,pending=0,stock=1)
        click(650,570);expect('guest other human remains read only',pending=0,stock=1)
        click(310,140);click(650,570);expect('guest own request',selected=2,pending=1,stock=1)
        key(0x7A);expect('native rejected edit preserves stock',pending=0,stock=1);capture('03-guest-rejected')
        key(0x73);expect('ownership-race pending fixture',scenario=4,pending=1,stock=1)
        key(ord('O'));expect('ownership change rejects pending host AI edit',pending=0,stock=1,equip=0);capture('ownership-loss')
        key(0x70);click(310,140);key(ord('D'));expect('disconnect retains soldier and permits host AI editor',selected=2,equip=1,stock=1);capture('disconnect-ai')
        key(0x70);key(ord('H'));expect('unassigned host can equip AI',equip=1,ready=0);capture('host-without-soldier')
        key(0x70);key(ord('R'));expect('revision-bound readiness',ready=2)
        click(650,570);key(0x79);expect('equipment commit clears readiness',ready=0,stock=0)
        key(ord('R'));expect('ready after essentials supplied',ready=2)
        click(750,706);expect('host deployment is queued',pending=1)
        key(0x79);expect('accepted deployment freezes preparation',pending=0,equip=0,phase=3);capture('deployed-frozen')
        for vk,number in [(0x72,3),(0x73,4),(0x74,5),(0x75,6),(0x76,7),(0x77,8),(0x78,9),(0x70,1)]:
            key(vk);expect(f'scenario {number}',scenario=number)
            capture(f'scenario-{number}')
        click(110,256);capture('briefing');click(110,256)
        click(720,256);capture('essentials');click(720,256)
        key(27);key(27);expect('Escape cancels confirmation',scenario=1)
        click(900,706);click(610,432);expect('mouse cancels leave',scenario=1)
        click(900,706);click(360,432)
        process.wait(timeout=10)
        if process.returncode: raise AssertionError(f'Preview exited {process.returncode}')
        Path(f'input-evidence-{args.width}x{args.height}.txt').write_text('\n'.join(f'{name}: {values}' for name,values in evidence)+'\n')
        print('PASS: native LS3D window, real mouse/keyboard input, nine scenarios and captures',flush=True)
    finally:
        if process.poll() is None: process.terminate(); process.wait(timeout=10)
        stream.close()

if __name__=='__main__':main()
