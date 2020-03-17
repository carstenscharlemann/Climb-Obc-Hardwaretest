import os
import serial

from Keyboard import KBHit

cliPort = '21'
if os.name == 'nt':
    cliPort = 'COM' + cliPort
else:
    cliPort = '/dev/ttyS' + cliPort

ser = serial.Serial(cliPort, 115200)  # open serial port
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
        ser.write(str.encode(c))
    else:
        out = ''
        while ser.inWaiting() > 0:
            out += ser.read(1).decode("utf-8")
        if out != '':
            print(out, end='')

kb.set_normal_term()