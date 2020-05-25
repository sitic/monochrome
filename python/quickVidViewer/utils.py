import flatbuffers
import numpy as np
from pathlib import Path
import socket
from typing import List, Text, Union, Optional

from .fbs import Root, Data, Filepaths, Array3Meta, Array3MetaFlow, Array3DataChunkf, Array3DataChunku16
from .fbs.ArrayDataType import ArrayDataType
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


def create_array3meta_msg(type: ArrayDataType, name, shape, duration=0., fps=0., date="", comment="",
                          bitrange=BitRange.AUTODETECT, cmap=ColorMap.DEFAULT, parentName=None):
    builder = flatbuffers.Builder(1024)
    name_fb = builder.CreateString(name)
    date_fb = builder.CreateString(date)
    comment_fb = builder.CreateString(comment)
    parent_fb = builder.CreateString(parentName) if parentName else None
    Array3Meta.Array3MetaStart(builder)
    Array3Meta.Array3MetaAddType(builder, type)
    Array3Meta.Array3MetaAddNx(builder, shape[2])
    Array3Meta.Array3MetaAddNy(builder, shape[1])
    Array3Meta.Array3MetaAddNt(builder, shape[0])
    Array3Meta.Array3MetaAddName(builder, name_fb)
    Array3Meta.Array3MetaAddDuration(builder, duration)
    Array3Meta.Array3MetaAddFps(builder, fps)
    Array3Meta.Array3MetaAddDate(builder, date_fb)
    Array3Meta.Array3MetaAddComment(builder, comment_fb)
    Array3Meta.Array3MetaAddBitrange(builder, bitrange)
    Array3Meta.Array3MetaAddCmap(builder, cmap)
    if parent_fb:
        Array3Meta.Array3MetaAddParentName(builder, parent_fb)
    d = Array3Meta.Array3MetaEnd(builder)

    root = build_root(builder, Data.Data.Array3Meta, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3metaflow_msg(shape, parentName=None, name=""):
    builder = flatbuffers.Builder(1024)
    name_fb = builder.CreateString(name)
    parent_fb = builder.CreateString(parentName) if parentName else None
    Array3MetaFlow.Array3MetaFlowStart(builder)
    Array3MetaFlow.Array3MetaFlowAddNx(builder, shape[2])
    Array3MetaFlow.Array3MetaFlowAddNy(builder, shape[1])
    Array3MetaFlow.Array3MetaFlowAddNt(builder, shape[0])
    Array3MetaFlow.Array3MetaFlowAddName(builder, name_fb)
    if parent_fb:
        Array3MetaFlow.Array3MetaFlowAddParentName(builder, parent_fb)
    d = Array3MetaFlow.Array3MetaFlowEnd(builder)

    root = build_root(builder, Data.Data.Array3MetaFlow, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3dataf_msg(array, idx=0):
    builder = flatbuffers.Builder(65536)
    data = builder.CreateNumpyVector(array)
    Array3DataChunkf.Array3DataChunkfStart(builder)
    Array3DataChunkf.Array3DataChunkfAddStartidx(builder, idx)
    Array3DataChunkf.Array3DataChunkfAddData(builder, data)
    d = Array3DataChunkf.Array3DataChunkfEnd(builder)

    root = build_root(builder, Data.Data.Array3DataChunkf, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3datau16_msg(array, idx=0):
    builder = flatbuffers.Builder(65536)
    data = builder.CreateNumpyVector(array)
    Array3DataChunku16.Array3DataChunku16Start(builder)
    Array3DataChunku16.Array3DataChunku16AddStartidx(builder, idx)
    Array3DataChunku16.Array3DataChunku16AddData(builder, data)
    d = Array3DataChunku16.Array3DataChunku16End(builder)

    root = build_root(builder, Data.Data.Array3DataChunku16, d)
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
                comment: Text = "", bitrange: BitRange = BitRange.AUTODETECT, cmap: ColorMap = ColorMap.DEFAULT,
                parentName: Optional[Text] = None):
    if array.ndim == 2:
        # assume that it is a 2D image
        array = np.expand_dims(array, 0)
    elif array.ndim != 3:
        raise ValueError("array is not three-dimensional")

    if array.dtype == np.float32:
        dtype = ArrayDataType.FLOAT
    elif array.dtype == np.uint16:
        dtype = ArrayDataType.UINT16
    else:
        if np.iscomplexobj(array):
            raise ValueError("Complex arrays not supported")
        else:
            array = array.astype(np.float32)
            dtype = ArrayDataType.FLOAT

    try:
        s = create_socket()
    except ConnectionRefusedError:
        print("Unable to connect to quickViewer")
        return
    buf = create_array3meta_msg(dtype, name, array.shape, duration_seconds, fps, date, comment, bitrange, cmap,
                                parentName)
    s.sendall(buf)

    flat = array.flatten()
    length = flat.size
    max_size = 16352
    for idx in range(0, length, max_size):
        end = length if idx + max_size > length else idx + max_size
        if array.dtype == np.float32:
            buf = create_array3dataf_msg(flat[idx:end], idx)
        else:
            buf = create_array3datau16_msg(flat[idx:end], idx)
        s.sendall(buf)


def open_flow(flow_uv: np.ndarray, parentName: Optional[Text] = None, name: Text = ""):
    if flow_uv.ndim != 4:
        raise ValueError("array is not three-dimensional")
    if flow_uv.dtype != np.float32:
        raise ValueError("array is not floating type")

    try:
        s = create_socket()
    except ConnectionRefusedError:
        print("Unable to connect to quickViewer")
        return

    flow_uv = np.moveaxis(flow_uv, -1, 0)
    shape = (flow_uv.shape[1] * 2, flow_uv.shape[2], flow_uv.shape[3])
    buf = create_array3metaflow_msg(shape, parentName)
    s.sendall(buf)

    flat = flow_uv.flatten()
    length = flat.size
    max_size = 16352
    for idx in range(0, length, max_size):
        end = length if idx + max_size > length else idx + max_size
        buf = create_array3dataf_msg(flat[idx:end], idx)
        s.sendall(buf)
