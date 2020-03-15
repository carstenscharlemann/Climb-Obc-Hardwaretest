import os
import serial
from stacie.stacie import StacieSim

from Keyboard import KBHit
cliPort = '21'

if os.name == 'nt':
    portPrefix = 'COM'
else:
    portPrefix = '/dev/ttyS'

cliPort = portPrefix + cliPort

#cliSer = serial.Serial(cliPort, 115200)  # open serial port

ttcA = StacieSim('ttca.json')
ttcC = StacieSim('ttcc.json')

kb = KBHit()

print('Hit any key, or ESC to exit')
while True:
    x = kb.kbhit()
    if x:
        c = kb.getch()
        lineend = ''
        o = ord(c)
        if ord(c) == 27:  # ESC
            break
        elif ord(c) == 13:
            lineend = '\n'
        # echo each char and send to UART ...
        print(c, end=lineend,  flush=True)
        #cliSer.write(str.encode(c))
    else:
        #out = ''
        #while cliSer.inWaiting() > 0:
        #    out += cliSer.read(1).decode("utf-8")
        #if out != '':
        #    print(out, end='')

        ttcA.main_non_blocking();
        ttcC.main_non_blocking();

kb.set_normal_term()