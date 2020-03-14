# %%
import flatbuffers
import fbs.Root
import fbs.Data
import fbs.Filepaths

# %%
builder = flatbuffers.Builder(512)
paths = ['/home/lebert/2019-09-25/MultiRecorder/2019-09-25_Exp002_Rec001_IDS_Cam2.dat']
paths_fb = [builder.CreateString(s) for s in paths]
fbs.Filepaths.FilepathsStartFileVector(builder, len(paths_fb))
for p in paths_fb:
    builder.PrependSOffsetTRelative(p)
vec = builder.EndVector(len(paths_fb))
fbs.Filepaths.FilepathsStart(builder)
fbs.Filepaths.FilepathsAddFile(builder, vec)
fp = fbs.Filepaths.FilepathsEnd(builder)

fbs.Root.RootStart(builder)
fbs.Root.RootAddDataType(builder, fbs.Data.Data.Filepaths)
fbs.Root.RootAddData(builder, fp)
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