import socket
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional, Text, Union

import flatbuffers
import numpy as np

# from .fbs import Root, Data, Filepaths, Array3Meta, Array3MetaFlow, Array3DataChunkf, Array3DataChunku16, PointsVideo, \
#     Points, Point
from .fbs import (Array3DataChunkf, Array3DataChunku16, Array3Meta,
                  Array3MetaFlow, Data, Filepaths, PointsVideo, Root)
from .fbs.ArrayDataType import ArrayDataType
from .fbs.BitRange import BitRange
from .fbs.Color import CreateColor
from .fbs.ColorMap import ColorMap
from .fbs.DictEntry import (DictEntryAddKey, DictEntryAddVal, DictEntryEnd,
                            DictEntryStart)
from .fbs.TransferFunction import TransferFunction

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


def get_color(builder, color):
    if color is None:
        return None

    try:
        import matplotlib.colors as mcolors
        color = mcolors.to_rgba(color)
    except ImportError:
        if isinstance(color, str):
            print("ERROR: unable to import matplotlib, please install it")
            color = (0.0, 0.0, 0.0, 1.0)
    return CreateColor(builder, color)


def build_root(builder, data_type, data):
    Root.Start(builder)
    Root.AddDataType(builder, data_type)
    Root.AddData(builder, data)
    root = Root.End(builder)
    return root


def create_filepaths_msg(paths):
    builder = flatbuffers.Builder(512)
    paths_fb = [builder.CreateString(s) for s in paths]
    Filepaths.StartFileVector(builder, len(paths_fb))
    for p in paths_fb:
        builder.PrependSOffsetTRelative(p)
    vec = builder.EndVector()
    Filepaths.Start(builder)
    Filepaths.AddFile(builder, vec)
    fp = Filepaths.End(builder)
    root = build_root(builder, Data.Data.Filepaths, fp)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


# def create_pointsvideo_msg(points_py, name, parent_name):
#     builder = flatbuffers.Builder(1024)
#     name_fb = builder.CreateString(name)
#     parent_fb = builder.CreateString(parent_name) if parent_name else None
#
#     points_video = []
#     for frame in points_py:
#         points_frame = []
#         for point in frame:
#             Point.Start(builder)
#             Point.AddX(builder, float(point[0]))
#             Point.AddY(builder, float(point[1]))
#             point_fb = Point.End(builder)
#             points_frame.append(point_fb)
#         Points.StartPointsVector(builder, len(points_frame))
#         for point in points_frame:
#             builder.PrependUOffsetTRelative(point)
#         points_video.append(builder.EndVector())
#
#     PointsVideo.StartPointsVideoVector(builder, len(points_video))
#     for p in points_video:
#         builder.PrependSOffsetTRelative(p)
#     video_vec = builder.EndVector()
#
#     PointsVideo.Start(builder)
#     PointsVideo.AddName(builder, name_fb)
#     if parent_fb:
#         PointsVideo.AddParentName(builder, parent_fb)
#     PointsVideo.AddPointsVideo(builder, video_vec)
#     fp = PointsVideo.End(builder)
#     root = build_root(builder, Data.Data.PointsVideo, fp)
#     builder.FinishSizePrefixed(root)
#     buf = builder.Output()
#     return buf

def create_pointsvideo_msg(points_py, name, parent_name=None, color=None):
    builder = flatbuffers.Builder(1024)
    name_fb = builder.CreateString(name)
    parent_fb = builder.CreateString(parent_name) if parent_name else None

    flat = []
    indexes = []
    for frame in points_py:
        for point in frame:
            flat.append(point[0])
            flat.append(point[1])
        indexes.append(len(flat))
    flat = np.array(flat, dtype=np.float32)
    indexs = np.array(indexes, dtype=np.uint32)
    flat_fb = builder.CreateNumpyVector(flat)
    indexes_fb = builder.CreateNumpyVector(indexs)

    PointsVideo.Start(builder)
    PointsVideo.AddName(builder, name_fb)
    if parent_fb:
        PointsVideo.AddParentName(builder, parent_fb)
    if color:
        PointsVideo.AddColor(builder, get_color(builder, color))
    PointsVideo.AddPointsData(builder, flat_fb)
    PointsVideo.AddTimeIdxs(builder, indexes_fb)
    fp = PointsVideo.End(builder)
    root = build_root(builder, Data.Data.PointsVideo, fp)
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
    Array3Meta.Start(builder)
    Array3Meta.AddType(builder, type)
    Array3Meta.AddNx(builder, shape[2])
    Array3Meta.AddNy(builder, shape[1])
    Array3Meta.AddNt(builder, shape[0])
    Array3Meta.AddName(builder, name_fb)
    Array3Meta.AddDuration(builder, duration)
    Array3Meta.AddFps(builder, fps)
    Array3Meta.AddDate(builder, date_fb)
    Array3Meta.AddComment(builder, comment_fb)
    Array3Meta.AddBitrange(builder, bitrange)
    Array3Meta.AddCmap(builder, cmap)
    if parent_fb:
        Array3Meta.AddParentName(builder, parent_fb)
    if transfer_fct:
        Array3Meta.AddAlphaTransferFct(builder, transfer_fct)
    if metaData:
        Array3Meta.AddMetaData(builder, metaData)
    d = Array3Meta.End(builder)

    root = build_root(builder, Data.Data.Array3Meta, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3metaflow_msg(shape, parentName=None, name="", color=None):
    builder = flatbuffers.Builder(1024)
    name_fb = builder.CreateString(name)
    parent_fb = builder.CreateString(parentName) if parentName else None
    Array3MetaFlow.Start(builder)
    Array3MetaFlow.AddNx(builder, shape[2])
    Array3MetaFlow.AddNy(builder, shape[1])
    Array3MetaFlow.AddNt(builder, shape[0])
    Array3MetaFlow.AddName(builder, name_fb)
    if parent_fb:
        Array3MetaFlow.AddParentName(builder, parent_fb)
    d = Array3MetaFlow.End(builder)

    root = build_root(builder, Data.Data.Array3MetaFlow, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3dataf_msg(array, idx=0):
    builder = flatbuffers.Builder(65536)
    data = builder.CreateNumpyVector(array)
    Array3DataChunkf.Start(builder)
    Array3DataChunkf.AddStartidx(builder, idx)
    Array3DataChunkf.AddData(builder, data)
    d = Array3DataChunkf.End(builder)

    root = build_root(builder, Data.Data.Array3DataChunkf, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3datau16_msg(array, idx=0):
    builder = flatbuffers.Builder(65536)
    data = builder.CreateNumpyVector(array)
    Array3DataChunku16.Start(builder)
    Array3DataChunku16.AddStartidx(builder, idx)
    Array3DataChunku16.AddData(builder, data)
    d = Array3DataChunku16.End(builder)

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


def show_points(points, name="", parent_name=None, color=None):
    """

    :param points:
    :param name:
    :param parent_name:
    :param color: Matplotlib color (either string like 'black' or rgb tuple)
    :return:
    """
    try:
        s = create_socket()
    except ConnectionRefusedError:
        print("Error: unable to connect to Monochrome")
        return
    buf = create_pointsvideo_msg(points, name, parent_name, color)
    s.sendall(buf)


def show_array(array: np.ndarray,
               name: Text = "",
               cmap: Union[ColorMap, Text] = ColorMap.DEFAULT,
               bitrange: Union[BitRange, Text] = BitRange.AUTODETECT,
               comment: Text = "",
               fps: float = 0,
               date: Text = "",
               duration_seconds: float = 0,
               parentName: Optional[Text] = None,
               transfer_fct: Optional[TransferFunction] = None,
               metaData: Optional[Dict] = None):
    """

    :param array: {t, x, y} ndarray
    :param name: name of the array
    :param cmap: 'default' (autodetect), 'gray', 'viridis', 'diff', 'hsv', or 'blackbody'
    :param bitrange: 'autodetect', 'uint8', 'uint10', 'uint12', 'uint16', 'float' (for [0,1]), 'diff', 'phase', or 'phase_diff'
    :param comment:
    :param fps: framerate in Hz
    :param date:
    :param duration_seconds:
    :param parentName:
    :param transfer_fct:
    :param metaData:
    :return:
    """
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

    if isinstance(cmap, str):
        cmap = getattr(ColorMap, cmap.upper())
    if isinstance(bitrange, str):
        bitrange = getattr(BitRange, bitrange.upper())

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
