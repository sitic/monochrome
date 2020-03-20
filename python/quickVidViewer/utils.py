import socket
import flatbuffers
import fbs.Root
import fbs.Data
import fbs.Filepaths
import fbs.Array3Meta
import fbs.Array3DataChunk
import numpy as np
from pathlib import Path

TCP_IP = '127.0.0.1'
TCP_PORT = 4864


def create_socket():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TCP_IP, TCP_PORT))
    return s


def build_root(builder, data_type, data):
    fbs.Root.RootStart(builder)
    fbs.Root.RootAddDataType(builder, data_type)
    fbs.Root.RootAddData(builder, data)
    root = fbs.Root.RootEnd(builder)
    return root


def create_filepaths_msg(paths):
    builder = flatbuffers.Builder(512)
    paths_fb = [builder.CreateString(s) for s in paths]
    fbs.Filepaths.FilepathsStartFileVector(builder, len(paths_fb))
    for p in paths_fb:
        builder.PrependSOffsetTRelative(p)
    vec = builder.EndVector(len(paths_fb))
    fbs.Filepaths.FilepathsStart(builder)
    fbs.Filepaths.FilepathsAddFile(builder, vec)
    fp = fbs.Filepaths.FilepathsEnd(builder)
    root = build_root(builder, fbs.Data.Data.Filepaths, fp)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3meta_msg(name, shape):
    builder = flatbuffers.Builder(1024)
    name_fb = builder.CreateString(name)
    fbs.Array3Meta.Array3MetaStart(builder)
    fbs.Array3Meta.Array3MetaAddName(builder, name_fb)
    fbs.Array3Meta.Array3MetaAddNt(builder, shape[0])
    fbs.Array3Meta.Array3MetaAddNy(builder, shape[2])
    fbs.Array3Meta.Array3MetaAddNx(builder, shape[1])
    d = fbs.Array3Meta.Array3MetaEnd(builder)

    root = build_root(builder, fbs.Data.Data.Array3Meta, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3data_msg(array):
    builder = flatbuffers.Builder(array.size * 4 + 128)
    data = builder.CreateNumpyVector(array.flatten())
    fbs.Array3DataChunk.Array3DataChunkStart(builder)
    fbs.Array3DataChunk.Array3DataChunkAddStartidx(builder, 0)
    fbs.Array3DataChunk.Array3DataChunkAddData(builder, data)
    d = fbs.Array3DataChunk.Array3DataChunkEnd(builder)

    root = build_root(builder, fbs.Data.Data.Array3DataChunk, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def send_filepaths(paths):
    paths = [Path(path) for path in paths]
    assert all([path.exists() for path in paths])
    paths = [str(path.absolute()) for path in paths]
    try:
        s = create_socket()
    except ConnectionRefusedError:
        print("Unable to connect to quickViewer")
        return
    buf = create_filepaths_msg(paths)
    s.sendall(buf)


def send_array3(name, array):
    assert array.ndim == 3
    assert array.dtype == np.float32

    try:
        s = create_socket()
    except ConnectionRefusedError:
        print("Unable to connect to quickViewer")
        return
    buf = create_array3meta_msg(name, array.shape)
    s.sendall(buf)
    buf = create_array3data_msg(array)
    s.sendall(buf)
