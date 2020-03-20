# %%
import numpy as np
import flatbuffers
import fbs.Root
import fbs.Data
import fbs.Array3Meta
import fbs.Array3DataChunk
import socket
TCP_IP = '127.0.0.1'
TCP_PORT = 4864
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))

shape = (1000, 128, 128)
arr = np.zeros(shape, dtype=np.float32)

# %%
builder = flatbuffers.Builder(1024)
name = 'TestArray'
name_fb = builder.CreateString(name)

fbs.Array3Meta.Array3MetaStart(builder)
fbs.Array3Meta.Array3MetaAddName(builder, name_fb)
fbs.Array3Meta.Array3MetaAddNt(builder, shape[0])
fbs.Array3Meta.Array3MetaAddNy(builder, shape[2])
fbs.Array3Meta.Array3MetaAddNx(builder, shape[1])
d = fbs.Array3Meta.Array3MetaEnd(builder)

fbs.Root.RootStart(builder)
fbs.Root.RootAddDataType(builder, fbs.Data.Data.Array3Meta)
fbs.Root.RootAddData(builder, d)
root = fbs.Root.RootEnd(builder)
builder.FinishSizePrefixed(root)
buf = builder.Output()
s.sendall(buf)

builder = flatbuffers.Builder(shape[0] * shape[1] * shape[2] * 4 + 128)
data = builder.CreateNumpyVector(arr.flatten())
fbs.Array3DataChunk.Array3DataChunkStart(builder)
fbs.Array3DataChunk.Array3DataChunkAddStartidx(builder, 0)
fbs.Array3DataChunk.Array3DataChunkAddData(builder, data)
d = fbs.Array3DataChunk.Array3DataChunkEnd(builder)

fbs.Root.RootStart(builder)
fbs.Root.RootAddDataType(builder, fbs.Data.Data.Array3DataChunk)
fbs.Root.RootAddData(builder, d)
root = fbs.Root.RootEnd(builder)
builder.FinishSizePrefixed(root)
buf = builder.Output()
s.sendall(buf)

s.close()
