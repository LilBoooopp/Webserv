import sys

for n in range(100000):
	sys.stdout.write("1")

while True:
    sys.stdout.write("0")
    sys.stdout.flush()