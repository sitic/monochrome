import flatbuffers
import numpy as np
from pathlib import Path
import socket
from typing import List, Text, Union

from .fbs import Root, Data, Filepaths, Array3Meta, Array3DataChunk
from .fbs.ColorMap import ColorMap
from .fbs.BitRange import BitRange

TCP_IP = '127.0.0.1'
TCP_PORT = 4864


def create_socket():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TCP_IP, TCP_PORT))
    return s


def build_root(builder, data_type, data):
    Root.RootStart(builder)
    Root.RootAddDataType(builder, data_type)
    Root.RootAddData(builder, data)
    root = Root.RootEnd(builder)
    return root


def create_filepaths_msg(paths):
    builder = flatbuffers.Builder(512)
    paths_fb = [builder.CreateString(s) for s in paths]
    Filepaths.FilepathsStartFileVector(builder, len(paths_fb))
    for p in paths_fb:
        builder.PrependSOffsetTRelative(p)
    vec = builder.EndVector(len(paths_fb))
    Filepaths.FilepathsStart(builder)
    Filepaths.FilepathsAddFile(builder, vec)
    fp = Filepaths.FilepathsEnd(builder)
    root = build_root(builder, Data.Data.Filepaths, fp)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3meta_msg(name, shape, duration=0., fps=0., date="", comment="",
                          bitrange=BitRange.AUTODETECT, cmap=ColorMap.DEFAULT):
    builder = flatbuffers.Builder(1024)
    name_fb = builder.CreateString(name)
    date_fb = builder.CreateString(date)
    comment_fb = builder.CreateString(comment)
    Array3Meta.Array3MetaStart(builder)
    Array3Meta.Array3MetaAddName(builder, name_fb)
    Array3Meta.Array3MetaAddNt(builder, shape[0])
    Array3Meta.Array3MetaAddNy(builder, shape[1])
    Array3Meta.Array3MetaAddNx(builder, shape[2])
    Array3Meta.Array3MetaAddDuration(builder, duration)
    Array3Meta.Array3MetaAddFps(builder, fps)
    Array3Meta.Array3MetaAddDate(builder, date_fb)
    Array3Meta.Array3MetaAddComment(builder, comment_fb)
    Array3Meta.Array3MetaAddBitrange(builder, bitrange)
    Array3Meta.Array3MetaAddCmap(builder, cmap)
    d = Array3Meta.Array3MetaEnd(builder)

    root = build_root(builder, Data.Data.Array3Meta, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3data_msg(array, idx=0):
    builder = flatbuffers.Builder(65536)
    data = builder.CreateNumpyVector(array)
    Array3DataChunk.Array3DataChunkStart(builder)
    Array3DataChunk.Array3DataChunkAddStartidx(builder, idx)
    Array3DataChunk.Array3DataChunkAddData(builder, data)
    d = Array3DataChunk.Array3DataChunkEnd(builder)

    root = build_root(builder, Data.Data.Array3DataChunk, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def open_file(filepath: Union[Text, Path]):
    open_files([filepath])


def open_files(paths: List[Union[Text, Path]]):
    paths = [Path(path) for path in paths]
    if not all([path.exists() for path in paths]):
        raise FileNotFoundError(f"One of more files of {paths} do not exist")
    paths = [str(path.absolute()) for path in paths]
    try:
        s = create_socket()
    except ConnectionRefusedError:
        print("Unable to connect to quickViewer")
        return
    buf = create_filepaths_msg(paths)
    s.sendall(buf)


def open_array3(array: np.ndarray, name: Text = "", duration_seconds: float = 0, fps: float = 0, date: Text = "",
                comment: Text = "", bitrange=BitRange.AUTODETECT, cmap=ColorMap.DEFAULT):
    if array.ndim != 3:
        raise ValueError("array is not three-dimensional")
    if array.dtype != np.float32:
        if np.iscomplexobj(array):
            raise ValueError("Complex arrays not supported")
        else:
            array = array.astype(np.float32)

    try:
        s = create_socket()
    except ConnectionRefusedError:
        print("Unable to connect to quickViewer")
        return
    buf = create_array3meta_msg(name, array.shape, duration_seconds, fps, date, comment, bitrange, cmap)
    s.sendall(buf)

    flat = array.flatten()
    length = flat.size
    max_size = 16352
    for idx in range(0, length, max_size):
        end = length if idx + max_size > length else idx + max_size
        buf = create_array3data_msg(flat[idx:end], idx)
        s.sendall(buf)
