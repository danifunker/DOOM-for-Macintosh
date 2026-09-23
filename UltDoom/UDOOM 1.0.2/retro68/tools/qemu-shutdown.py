#!/usr/bin/env python3
"""Shut the emulated Mac down gracefully so its disk caches reach the disk
images: quit DOOM if it's running (Cmd-Q, Y), then Special > Shut Down in
the Finder, driven with the mouse and checked on screen.  QEMU's Quadra
powers off by itself at the end."""
import subprocess, time, os, sys
from PIL import Image
os.chdir(os.path.dirname(os.path.abspath(__file__)))

def qm(*cmds):
    subprocess.run(['./qm.py', *cmds], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def running():
    # our QEMU: the one started in this directory (others may be running)
    here = os.getcwd()
    for pid in subprocess.run(['pgrep', 'qemu-system-m68'], capture_output=True,
                              text=True).stdout.split():
        try:
            if os.readlink('/proc/%s/cwd' % pid) == here:
                return True
        except OSError:
            pass
    return False

def shot():
    qm('shot sd'); time.sleep(0.3)
    return Image.open('sd.png').convert('RGB')

def dark(im, x0, x1, y0, y1):
    return sum(1 for x in range(x0, x1) for y in range(y0, y1)
               if sum(im.getpixel((x, y))) < 200)

def in_game(im):            # the game's "Control" menu title (launchgame.py)
    return dark(im, 196, 206, 3, 15) > 5

def moves(dx, dy, n):
    c = []
    for _ in range(n):
        c += ['mouse_move %d %d' % (dx, dy), 'sleep 0.05']
    qm(*c)

def main():
    if not os.path.exists('mon.sock') or not running():
        return 0
    for attempt in range(4):
        if not in_game(shot()):
            break
        # DOOM asks "are you sure?" (Y); the shareware build then shows an
        # order dialog whose default button is Quit (Return).
        qm('mouse_button 0', 'sendkey esc', 'sleep 1', 'sendkey meta_l-q',
           'sleep 2', 'sendkey y', 'sleep 3', 'sendkey ret', 'sleep 6')
    subprocess.run(['./waitfinder.sh', '30'], stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)
    for attempt in range(3):
        qm('mouse_button 0')
        moves(-300, -300, 4)            # park in the top-left corner
        time.sleep(2.5)                 # ADB acceleration settles
        moves(10, 0, 33)                # to "Special"
        qm('mouse_button 1', 'sleep 0.5')
        moves(0, 10, 8)                 # part of the way down the menu
        hit = False
        passed = False
        for step in range(30):          # then a notch at a time
            time.sleep(0.2)
            im = shot()
            if dark(im, 205, 330, 134, 145) > 800:      # "Shut Down" lit
                hit = True
                break
            lit = [y for y in range(22, 146, 4) if dark(im, 205, 330, y, y + 3) > 300]
            if lit and lit[0] >= 118:   # "Restart": Shut Down is next
                passed = True
            if lit or not passed:       # above it (or on a separator line)
                moves(0, 10, 1)
            else:                       # below the menu
                moves(0, -10, 1)
        if hit:
            qm('mouse_button 0')
            break
        qm('mouse_button 0', 'sendkey esc')
        time.sleep(1)
    for _ in range(90):
        if not running():
            print('shut down')
            return 0
        time.sleep(1)
    qm('quit')                          # "You may now switch off" etc.
    print('shut down (QEMU stopped at the end)')
    return 0

sys.exit(main())
