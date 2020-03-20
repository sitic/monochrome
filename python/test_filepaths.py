# %%
import flatbuffers
import fbs.Root
import fbs.Data
import fbs.Filepaths

# %%
builder = flatbuffers.Builder(512)
paths = ['/home/sitic/Programming/quickmultrecviewer/2019-09-24_Exp001_Rec022_IDS_Cam1.dat']
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
builder.FinishSizePrefixed(root)

buf = builder.Output()
print(len(buf))

import socket
TCP_IP = '127.0.0.1'
TCP_PORT = 4864

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
s.sendall(buf)
s.close()
