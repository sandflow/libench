import os

stream = os.popen("./build/libench ojph images/rgba.png")
output = stream.read()
print(output)
