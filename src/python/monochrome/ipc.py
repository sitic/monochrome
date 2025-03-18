import os
import socket
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional, Text, Union

import flatbuffers
import numpy as np

from .fbs import (
    Array3DataChunkf,
    Array3DataChunku8,
    Array3DataChunku16,
    Array3Meta,
    Array3MetaFlow,
    CloseVideo,
    Filepaths,
    PointsVideo,
    Root,
    VideoExport,
)
from .fbs.ArrayDataType import ArrayDataType
from .fbs.BitRange import BitRange
from .fbs.Color import CreateColor
from .fbs.ColorMap import ColorMap
from .fbs.Data import Data
from .fbs.DictEntry import DictEntryAddKey, DictEntryAddVal, DictEntryEnd, DictEntryStart
from .fbs.OpacityFunction import OpacityFunction
from .fbs.VideoExportFormat import VideoExportFormat

if sys.platform == "win32":
    MONOCHROME_BIN_PATH = Path(__file__).parent / "data" / "bin" / "Monochrome.exe"
elif sys.platform == "darwin":
    MONOCHROME_BIN_PATH = Path(__file__).parent / "data" / "Monochrome.app"
else:
    MONOCHROME_BIN_PATH = Path(__file__).parent / "data" / "bin" / "Monochrome"

USE_TCP = sys.platform in ["win32", "cygwin"]
TCP_IP, TCP_PORT = "127.0.0.1", 4864
# OSX doesn't support abstract UNIX domain sockets
ABSTRACT_DOMAIN_SOCKET_SUPPORTED = sys.platform != "darwin"
if sys.platform != "win32":
    SOCK_PATH = f"\0Monochrome{os.getuid()}" if ABSTRACT_DOMAIN_SOCKET_SUPPORTED else f"/tmp/Monochrome{os.getuid()}.s"
else:
    SOCK_PATH = None
MAX_BUFFER_SIZE = 16352
MONOCHROME_DEFAULT_ARGS = {}


def start_monochrome(speed: Optional[float] = None,
                     display_fps: Optional[int] = None,
                     scale: Optional[float] = None,
                     fliph: bool = False,
                     flipv: bool = False,
                     **kwargs):
    """Start bundled Monochrome executable with the given settings."""
    args = []
    if speed:
        args.append("--speed")
        args.append(str(speed))
    if display_fps:
        args.append("--display_fps")
        args.append(str(display_fps))
    if scale:
        args.append("--scale")
        args.append(str(scale))
    if fliph:
        args.append("--fliph")
    if flipv:
        args.append("--flipv")
    kwargs = {**MONOCHROME_DEFAULT_ARGS, **kwargs}
    for key, val in kwargs.items():
        args.append(f"--{key}")
        if isinstance(val, bool):
            pass
        else:
            args.append(str(val))

    if sys.platform == "darwin" and '--unit-test-mode' in args:
        cmd = [str(MONOCHROME_BIN_PATH / 'Contents' / 'MacOS' / 'Monochrome'), ] + args
    elif sys.platform == "darwin":
        cmd = ["open", "-a", str(MONOCHROME_BIN_PATH), "--args", ] + args
    else:
        cmd = [str(MONOCHROME_BIN_PATH), ] + args
    subprocess.Popen(cmd, start_new_session=True)


def console_entrypoint():
    if sys.platform != "darwin":
        args = [str(MONOCHROME_BIN_PATH)]
    else:
        args = ["open", "-a", str(MONOCHROME_BIN_PATH), "--args"]
    args.extend(sys.argv[1:])
    subprocess.Popen(args).wait()


def _create_socket():
    if USE_TCP:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((TCP_IP, TCP_PORT))
    else:
        if not ABSTRACT_DOMAIN_SOCKET_SUPPORTED and not Path(SOCK_PATH).exists():
            raise ConnectionRefusedError("No socket found, please start Monochrome")
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.connect(SOCK_PATH)
    return s


def create_socket():
    s = None
    try:
        s = _create_socket()
    except ConnectionRefusedError:
        if not USE_TCP and not ABSTRACT_DOMAIN_SOCKET_SUPPORTED:
            Path(SOCK_PATH).unlink(missing_ok=True)
        start_monochrome()
        waiting = True
        timeout = time.time() + 5
        while waiting and time.time() < timeout:
            try:
                s = _create_socket()
                waiting = False
            except ConnectionRefusedError:
                pass
        if waiting:
            raise ConnectionRefusedError("Could not connect to Monochrome")
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
    root = build_root(builder, Data.Filepaths, fp)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_pointsvideo_msg(points_py, name, parent_name=None, color=None, point_size=None):
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
    if point_size:
        PointsVideo.AddPointSize(builder, point_size)
    PointsVideo.AddPointsData(builder, flat_fb)
    PointsVideo.AddTimeIdxs(builder, indexes_fb)
    fp = PointsVideo.End(builder)
    root = build_root(builder, Data.PointsVideo, fp)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf

def create_array3meta_msg(dtype: ArrayDataType, name, shape, duration=0., fps=0., date="", comment="",
                          bitrange=BitRange.AUTODETECT, cmap=ColorMap.DEFAULT, parent_name=None, opacity=None,
                          metadata=None, vmin=None, vmax=None):
    builder = flatbuffers.Builder(1024)
    name_fb = builder.CreateString(name)
    parent_fb = builder.CreateString(parent_name) if parent_name is not None else None
    date_fb = builder.CreateString(date)
    comment_fb = builder.CreateString(comment)
    if metadata:
        metadata = [(builder.CreateString(key), builder.CreateString(str(val))) for key, val in metadata.items()]
        metaData_fbs = []
        for key, val in metadata:
            DictEntryStart(builder)
            DictEntryAddKey(builder, key)
            DictEntryAddVal(builder, val)
            metaData_fbs.append(DictEntryEnd(builder))
        Array3Meta.Array3MetaStartMetadataVector(builder, len(metadata))
        for e in metaData_fbs:
            builder.PrependUOffsetTRelative(e)
        metadata = builder.EndVector()
    Array3Meta.Start(builder)
    Array3Meta.AddType(builder, dtype)
    Array3Meta.AddNx(builder, shape[2])
    Array3Meta.AddNy(builder, shape[1])
    Array3Meta.AddNt(builder, shape[0] * shape[3])
    Array3Meta.AddBitrange(builder, bitrange)
    Array3Meta.AddCmap(builder, cmap)
    if vmin is not None:
        Array3Meta.AddVmin(builder, vmin)
    if vmax is not None:
        Array3Meta.AddVmax(builder, vmax)
    if opacity is not None:
        Array3Meta.AddOpacity(builder, opacity)
    Array3Meta.AddName(builder, name_fb)
    if parent_fb:
        Array3Meta.AddParentName(builder, parent_fb)
    Array3Meta.AddDuration(builder, duration)
    Array3Meta.AddFps(builder, fps)
    Array3Meta.AddDate(builder, date_fb)
    Array3Meta.AddComment(builder, comment_fb)
    if metadata:
        Array3Meta.AddMetadata(builder, metadata)
    Array3Meta.AddNc(builder, shape[3])
    d = Array3Meta.End(builder)

    root = build_root(builder, Data.Array3Meta, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3metaflow_msg(shape, parent_name=None, name="", color=None):
    if parent_name is None:
        parent_name = ""
    builder = flatbuffers.Builder(1024)
    name_fb = builder.CreateString(name)
    parent_fb = builder.CreateString(parent_name)
    Array3MetaFlow.Start(builder)
    Array3MetaFlow.AddNx(builder, shape[2])
    Array3MetaFlow.AddNy(builder, shape[1])
    Array3MetaFlow.AddNt(builder, shape[0])
    Array3MetaFlow.AddName(builder, name_fb)
    Array3MetaFlow.AddParentName(builder, parent_fb)
    if color:
        Array3MetaFlow.AddColor(builder, get_color(builder, color))
    d = Array3MetaFlow.End(builder)

    root = build_root(builder, Data.Array3MetaFlow, d)
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

    root = build_root(builder, Data.Array3DataChunkf, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def create_array3datau8_msg(array, idx=0):
    builder = flatbuffers.Builder(65536)
    data = builder.CreateNumpyVector(array)
    Array3DataChunku8.Start(builder)
    Array3DataChunku8.AddStartidx(builder, idx)
    Array3DataChunku8.AddData(builder, data)
    d = Array3DataChunku8.End(builder)

    root = build_root(builder, Data.Array3DataChunku8, d)
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

    root = build_root(builder, Data.Array3DataChunku16, d)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    return buf


def show_file(filepath: Union[Text, Path]):
    """
    Load a file in Monochrome.

    Parameters
    ----------
    filepath : str or Path
        Path to the file to be loaded
    """
    show_files([filepath])


def show_files(paths: List[Union[Text, Path]]):
    """
    Load multiple files in Monochrome.

    Parameters
    ----------
    paths : List[str or Path]
        List of paths to the files to be loaded
    """
    paths = [Path(path) for path in paths]
    if not all([path.exists() for path in paths]):
        raise FileNotFoundError(f"One of more files of {paths} do not exist")
    paths = [str(path.absolute()) for path in paths]
    s = create_socket()
    buf = create_filepaths_msg(paths)
    s.sendall(buf)


def show_points(points, name: Text = "", parent: Optional[Text] = None, color=None,
                point_size: Optional[float] = None):
    """
    Show a list of points for each frame in Monochrome.

    Parameters
    ----------
    points : List[List[Tuple[float, float]]]
        A list of list of points (x, y). The outer list elements are the frames, the inner list is the list of points for a specific frame.
    name : str
        Optional description
    parent : str
        Name of the video onto which the points will be displayed. If none is given the last loaded video will be used.
    color : str or tuple
        Matplotlib color (either string like 'black' or rgba tuple)
    point_size : float
        Size of points in image pixels
    """
    name = str(name)
    s = create_socket()
    buf = create_pointsvideo_msg(points, name, parent, color, point_size)
    s.sendall(buf)


def show_image(array: np.ndarray,
               name: Text = "",
               cmap: Union[ColorMap, Text] = ColorMap.DEFAULT,
               vmin: Optional[float] = None,
               vmax: Optional[float] = None,
               bitrange: Union[BitRange, Text] = BitRange.AUTODETECT,
               parent: Optional[Text] = None,
               opacity: Optional[OpacityFunction] = None,
               comment: Text = "",
               metadata: Optional[Dict] = None):
    """
    Show an image in Monochrome.

    Alias for :func:`show_video`.
    """
    return show_video(array, name=name, cmap=cmap, vmin=vmin, vmax=vmax, bitrange=bitrange, parent=parent, opacity=opacity, comment=comment, metadata=metadata)


def show_video(array: np.ndarray,
               name: Text = "",
               cmap: Union[ColorMap, Text] = ColorMap.DEFAULT,
               vmin: Optional[float] = None,
               vmax: Optional[float] = None,
               bitrange: Union[BitRange, Text] = BitRange.AUTODETECT,
               parent: Optional[Text] = None,
               opacity: Optional[OpacityFunction] = None,
               comment: Text = "",
               metadata: Optional[Dict] = None):
    """Play a video or open a image in Monochrome.

    Arrays of dtype np.float, np.uint8, and np.uint16 are natively supported by Monochrome.
    Arrays with other dtypes will be converted to np.float32.

    Parameters
    ----------
    array : np.ndarray
        The video to be displayed. The array should have the shape (T, H, W) or (H, W).
    name : str
        Name of the video
    cmap : str or ColorMap
        Colormap for the video. One of 'default' (autodetect), 'gray', 'hsv', 'blackbody', 'viridis', 'PRGn', 'PRGn_pos', 'PRGn_neg', 'RdBu', 'tab10'.
    vmin : float
        Minimum value for the colormap. Default is None.
    vmax : float
        Maximum value for the colormap. Default is None.
    bitrange : str or BitRange
        Valuerange for the video. One of 'autodetect', 'MinMax' 'uint8', 'uint10', 'uint12', 'uint16', 'float' (for [0,1]), 'diff' (for [-1, 1]), 'phase' (for [0, 2*pi]), or 'phase_diff (for [-pi, pi])'. Default is 'autodetect'.
    parent : str
        Name of the parent video
    opacity : OpacityFunction
        Opacity function for alpha blending if video is a layer. One of 'linear', 'linear_r', 'centered', 1.0, 0.75, 0.5, 0.25, or 0.0. Default is `opacity=1.0`.
    comment : str
        Comment to be displayed
    metadata : dict
        Additional metadata to be displayed
    """
    array = np.squeeze(array)
    if array.ndim == 2:
        # assume that it is a 2D image
        array = array[np.newaxis, :, :, np.newaxis]
    elif array.ndim == 3:
        if array.shape[2] in (3, 4):
            # assume that it is an RGB(A) image
            if array.shape[2] == 4:
                # alpha channel is not yet supported
                array = array[..., :3]
            array = np.expand_dims(array, 0)
        else:
            array = np.expand_dims(array, -1)
    elif array.ndim == 4:
        if array.shape[3] not in (3, 4):
            msg = f"Video has an unsupported shape {array.shape}. Four dimensional arrays are only supported if the last dimension is 3 (RGB) or 4 (RGBA)."
            raise ValueError(msg)
    else:
        msg = f"Array does not have an image/video shape: Shape {array.shape}"
        raise ValueError(msg)

    if array.dtype == np.float32:
        dtype = ArrayDataType.FLOAT
    elif array.dtype == np.uint8:
        dtype = ArrayDataType.UINT8
    elif array.dtype == np.uint16:
        dtype = ArrayDataType.UINT16
    else:
        if np.iscomplexobj(array):
            raise ValueError("Complex arrays not supported")
        else:
            array = array.astype(np.float32)
            dtype = ArrayDataType.FLOAT

    name = str(name)

    if isinstance(cmap, str):
        cmap = getattr(ColorMap, cmap.upper())
    if isinstance(bitrange, str):
        bitrange = getattr(BitRange, bitrange.upper())
    if isinstance(opacity, str):
        try:
            opacity = float(opacity)
        except ValueError:
            pass
    if isinstance(opacity, (int, float)):
        if opacity == 1:
            opacity = OpacityFunction.FIXED_100
        elif opacity == 0.75:
            opacity = OpacityFunction.FIXED_75
        elif opacity == 0.5:
            opacity = OpacityFunction.FIXED_50
        elif opacity == 0.25:
            opacity = OpacityFunction.FIXED_25
        elif opacity == 0:
            opacity = OpacityFunction.FIXED_0
        else:
            raise ValueError("Invalid opacity value")
    if isinstance(opacity, str):
        opacity = getattr(OpacityFunction, opacity.upper())

    s = create_socket()
    buf = create_array3meta_msg(dtype, name, array.shape, comment=comment, bitrange=bitrange, cmap=cmap,
                                parent_name=parent, opacity=opacity, metadata=metadata, vmin=vmin, vmax=vmax)
    s.sendall(buf)

    flat = array.flatten()
    length = flat.size
    max_size = MAX_BUFFER_SIZE
    for idx in range(0, length, max_size):
        end = length if idx + max_size > length else idx + max_size
        if array.dtype == np.float32:
            buf = create_array3dataf_msg(flat[idx:end], idx)
        elif array.dtype == np.uint8:
            buf = create_array3datau8_msg(flat[idx:end], idx)
        elif array.dtype == np.uint16:
            buf = create_array3datau16_msg(flat[idx:end], idx)
        else:
            raise NotImplementedError("Unkown dtype")
        s.sendall(buf)


def show_layer(array: np.ndarray, name: Text = "", parent: Optional[Text] = None, opacity: Optional[OpacityFunction] = None, **kwargs):
    """
    Add a layer to the parent video in Monochrome.

    Parameters
    ----------
    array : np.ndarray
        The layer to be displayed. The array should have the shape (T, H, W) or (H, W).
    name : str
        Name of the layer
    parent : str
        Name of the parent video, if None the last loaded video will be used
    opacity : OpacityFunction
        Opacity function for alpha blending. One of 'linear', 'linear_r', 'centered', 1.0, 0.75, 0.5, 0.25, or 0.0. Default is `opacity=1.0`.
    kwargs : dict
        Additional arguments to be passed to :func:`show_video`
    """
    if parent is None:
        parent = ""
    show_video(array, name=name, parent=parent, opacity=opacity, **kwargs)


def show_flow(flow_uv: np.ndarray, name: Text = "", parent: Optional[Text] = None, color=None):
    """
    Visualize optical flow in Monochrome.

    Parameters
    ----------
    flow_uv : np.ndarray
        Optical flow field of shape (T, H, W, 2)
    name : str
        Name of the flow
    parent : str
        Name of the parent video, if None the last loaded video will be used
    color : str or tuple
        Matplotlib color (either string like 'black' or rgba tuple)
    """
    if flow_uv.ndim != 4:
        raise ValueError("array is not four-dimensional")
    if flow_uv.dtype != np.float32:
        raise ValueError("array is not floating type")
    if flow_uv.shape[3] != 2:
        raise ValueError("flow should be of shape [T, H, W, 2]")
    name = str(name)

    s = create_socket()
    shape = (flow_uv.shape[0] * 2, flow_uv.shape[1], flow_uv.shape[2])
    buf = create_array3metaflow_msg(shape, parent, name, color)
    s.sendall(buf)

    flat = flow_uv.flatten()
    length = flat.size
    max_size = MAX_BUFFER_SIZE
    for idx in range(0, length, max_size):
        end = length if idx + max_size > length else idx + max_size
        buf = create_array3dataf_msg(flat[idx:end], idx)
        s.sendall(buf)


def show(array_or_path: Union[str, Path, np.ndarray], *args, **kwargs):
    """Autodetect the type of the input and show it in Monochrome."""
    if isinstance(array_or_path, np.ndarray):
        if array_or_path.ndim == 4 and array_or_path.shape[3] == 2:
            return show_flow(array_or_path, *args, **kwargs)
        else:
            return show_video(array_or_path, *args, **kwargs)
    elif isinstance(array_or_path, str) or isinstance(array_or_path, Path):
        return show_file(array_or_path)
    else:
        raise ValueError("array_or_path has to be numpy array or string")

def export_video(filepath, name="", fps=30, t_start=0, t_end=-1, description="", close_after_completion=False):
    """Export a video displayed in Monochrome to a .mp4 file.

    .. note::
        Monochrome exports the video as rendered in the window, i.e. the video will have the same resolution as
        the video window and all the layers/points/... will be merged into a single video.

    Parameters
    ----------
    filepath : str
        Path to the output .mp4 file
    name : str
        Name of the video to be exported
    fps : int
        Frames per second of the output video
    t_start : int
        Start frame of the output video
    t_end : int
        End frame of the output video, -1 for the last frame
    description : str
        Description of the video to embed in the .mp4 file
    close_after_completion : bool
        Close the video in Monochrome after the export is completed
    """
    s = create_socket()
    builder = flatbuffers.Builder(512)
    name_fb = builder.CreateString(name)
    filepath_fb = builder.CreateString(str(Path(filepath).absolute()))
    description_fb = builder.CreateString(description)

    VideoExport.Start(builder)
    VideoExport.AddRecording(builder, name_fb)
    VideoExport.AddFilepath(builder, filepath_fb)
    VideoExport.AddDescription(builder, description_fb)
    VideoExport.AddFormat(builder, VideoExportFormat.FFMPEG)
    VideoExport.AddFps(builder, fps)
    VideoExport.AddTStart(builder, t_start)
    VideoExport.AddTEnd(builder, t_end)
    VideoExport.AddCloseAfterCompletion(builder, close_after_completion)
    fp = VideoExport.End(builder)
    root = build_root(builder, Data.VideoExport, fp)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    s.sendall(buf)

def close_video(name=""):
    """Close a video in Monochrome.

    Parameters
    ----------
    name : str
        Name of the video to be closed. If empty, the last loaded video will be closed.
    """
    s = create_socket()
    builder = flatbuffers.Builder(512)
    name_fb = builder.CreateString(name)

    CloseVideo.Start(builder)
    CloseVideo.AddName(builder, name_fb)
    fp = CloseVideo.End(builder)
    root = build_root(builder, Data.CloseVideo, fp)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    s.sendall(buf)

def quit():  # noqa: A001
    """Quit Monochrome, terminating the process."""
    s = create_socket()
    builder = flatbuffers.Builder(512)
    root = build_root(builder, Data.Quit, 0)
    builder.FinishSizePrefixed(root)
    buf = builder.Output()
    s.sendall(buf)
