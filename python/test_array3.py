# %%
import numpy as np
import flatbuffers
import fbs.Root
import fbs.Data
import fbs.Array3

shape = (100, 128, 128)
arr = np.zeros(shape, dtype=np.float32)

# %%
builder = flatbuffers.Builder(1024)
name = 'TestArray'
name_fb = builder.CreateString(name)
data = builder.CreateNumpyVector(arr.flatten())

fbs.Array3.Array3Start(builder)
fbs.Array3.Array3AddName(builder, name_fb)
fbs.Array3.Array3AddNt(builder, shape[0])
fbs.Array3.Array3AddNy(builder, shape[2])
fbs.Array3.Array3AddNx(builder, shape[1])
fbs.Array3.Array3AddData(builder, data)
d = fbs.Array3.Array3End(builder)

fbs.Root.RootStart(builder)
fbs.Root.RootAddDataType(builder, fbs.Data.Data.Array3)
fbs.Root.RootAddData(builder, d)
root = fbs.Root.RootEnd(builder)
builder.Finish(root)

buf = builder.Output()
print(len(buf))

import socket
TCP_IP = '127.0.0.1'
TCP_PORT = 4864
BUFFER_SIZE = 1024

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
s.send(buf)
s.close()