import sys
import time

with open("/dev/tty", "w") as tty:
    while True:
        tty.write("infinite.py is running\n")
        tty.flush()
        time.sleep(1)
