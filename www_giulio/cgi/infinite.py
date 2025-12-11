import sys

with open("/dev/tty", "w") as tty:
    while True:
        tty.write(".")
        tty.flush()
