import flatbuffers
import numpy as np
from pathlib import Path
import socket
import sys
from typing import List, Text, Union, Optional, Dict

from .fbs import Root, Data, Filepaths, Array3Meta, Array3MetaFlow, Array3DataChunkf, Array3DataChunku16
from .fbs.ArrayDataType import ArrayDataType
from .fbs.ColorMap import ColorMap
from .fbs.BitRange import BitRange
from .fbs.TransferFunction import TransferFunction
from .fbs.DictEntry import DictEntryStart, DictEntryAddKey, DictEntryAddVal, DictEntryEnd

TCP_IP, TCP_PORT = '127.0.0.1', 4864
# OSX doesn't support abstract UNIX domain sockets
SOCK_PATH = '/tmp/Monochrome.s' if sys.platform == 'darwin' else '\0Monochrome'
USE_TCP = sys.platform in ['win32', 'cygwin']


def create_socket():
    if USE_TCP:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((TCP_IP, TCP_PORT))
    else:
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.connect(SOCK_PATH)
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
    vec = builder.EndVector()
    Filepaths.FilepathsStart(builder)
    Filepaths.FilepathsAddFile(builder, vec)
    fp = Filepaths.FilepathsEnd(builder)
    root = build_root(builder, Data.Data.Filepaths, fp)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3meta_msg(type: ArrayDataType, name, shape, duration=0., fps=0., date="", comment="",
                          bitrange=BitRange.AUTODETECT, cmap=ColorMap.DEFAULT, parentName=None, transfer_fct=None,
                          metaData=None):
    builder = flatbuffers.Builder(1024)
    name_fb = builder.CreateString(name)
    date_fb = builder.CreateString(date)
    comment_fb = builder.CreateString(comment)
    parent_fb = builder.CreateString(parentName) if parentName else None
    if metaData:
        metaData = [(builder.CreateString(key), builder.CreateString(val)) for key, val in metaData.items()]
        metaData_fbs = []
        for key, val in metaData:
            DictEntryStart(builder)
            DictEntryAddKey(builder, key)
            DictEntryAddVal(builder, val)
            metaData_fbs.append(DictEntryEnd(builder))
        Array3Meta.Array3MetaStartMetaDataVector(builder, len(metaData))
        for e in metaData_fbs:
            builder.PrependUOffsetTRelative(e)
        metaData = builder.EndVector()
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
    if transfer_fct:
        Array3Meta.Array3MetaAddAlphaTransferFct(builder, transfer_fct)
    if metaData:
        Array3Meta.Array3MetaAddMetaData(builder, metaData)
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


def show_file(filepath: Union[Text, Path]):
    show_files([filepath])


def show_files(paths: List[Union[Text, Path]]):
    paths = [Path(path) for path in paths]
    if not all([path.exists() for path in paths]):
        raise FileNotFoundError(f"One of more files of {paths} do not exist")
    paths = [str(path.absolute()) for path in paths]
    try:
        s = create_socket()
    except ConnectionRefusedError:
        print("Error: unable to connect to Monochrome")
        return
    buf = create_filepaths_msg(paths)
    s.sendall(buf)


def show_array(array: np.ndarray, name: Text = "", duration_seconds: float = 0, fps: float = 0, date: Text = "",
               comment: Text = "", bitrange: BitRange = BitRange.AUTODETECT, cmap: ColorMap = ColorMap.DEFAULT,
               parentName: Optional[Text] = None, transfer_fct: Optional[TransferFunction] = None,
               metaData: Optional[Dict] = None):
    array = np.squeeze(array)
    if array.ndim == 2:
        # assume that it is a 2D image
        array = np.expand_dims(array, 0)
    elif array.ndim != 3:
        raise ValueError("array is not two- or three-dimensional")

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
        print("Error: unable to connect to Monochrome")
        return
    buf = create_array3meta_msg(dtype, name, array.shape, duration=duration_seconds, fps=fps, date=date,
                                comment=comment, bitrange=bitrange, cmap=cmap, parentName=parentName,
                                transfer_fct=transfer_fct, metaData=metaData)
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


def show_layer(array: np.ndarray, parentName: Text, name: Text = "", **kwargs):
    show_array(array, parentName=parentName, name=name, **kwargs)


def show_flow(flow_uv: np.ndarray, parentName: Optional[Text] = None, name: Text = ""):
    if flow_uv.ndim != 4:
        raise ValueError("array is not four-dimensional")
    if flow_uv.dtype != np.float32:
        raise ValueError("array is not floating type")
    if flow_uv.shape[3] != 2:
        raise ValueError("flow should be of shape [T, H, W, 2]")

    try:
        s = create_socket()
    except ConnectionRefusedError:
        print("Error: unable to connect to Monochrome")
        return

    shape = (flow_uv.shape[0] * 2, flow_uv.shape[1], flow_uv.shape[2])
    buf = create_array3metaflow_msg(shape, parentName, name)
    s.sendall(buf)

    flat = flow_uv.flatten()
    length = flat.size
    max_size = 16352
    for idx in range(0, length, max_size):
        end = length if idx + max_size > length else idx + max_size
        buf = create_array3dataf_msg(flat[idx:end], idx)
        s.sendall(buf)


def show(array_or_path: Union[str, np.ndarray], *args, **kwargs):
    if isinstance(array_or_path, np.ndarray):
        return show_array(array_or_path, *args, **kwargs)
    elif isinstance(array_or_path, str) or isinstance(array_or_path, Path):
        return show_file(array_or_path)
    else:
        raise ValueError("array_or_path has to be numpy array or string")
