import serial
import sys
import select


ser = serial.Serial('/dev/ttyS21',115200)  # open serial port

read_list = [sys.stdin]
timeout = 0.1 # seconds 
while read_list:
    ready = select.select(read_list, [], [], timeout)[0]
    if not ready:		# no input from Stdio there -> read Serial
        out = ''
        while ser.inWaiting()>0:
            out += ser.read(1).decode("utf-8")
        if out != '':
            print(out, end = '')
    else:
        for file in ready:
            line = file.readline()
            if not line:
                read_list.remove(file)
            else:
                ser.write(str.encode(line))
