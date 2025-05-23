# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
from typing import Any
from .DictEntry import DictEntry
from typing import Optional
np = import_numpy()

class Array3Meta(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset: int = 0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Array3Meta()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsArray3Meta(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Array3Meta
    def Init(self, buf: bytes, pos: int):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Array3Meta
    def Type(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def Nx(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def Ny(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def Nt(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(10))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def Bitrange(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(12))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def Cmap(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(14))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def Vmin(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(16))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Float32Flags, o + self._tab.Pos)
        return 0.0

    # Array3Meta
    def Vmax(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(18))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Float32Flags, o + self._tab.Pos)
        return 0.0

    # Array3Meta
    def Opacity(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(20))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def Name(self) -> Optional[str]:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(22))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def ParentName(self) -> Optional[str]:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(24))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def Duration(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(26))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Float32Flags, o + self._tab.Pos)
        return 0.0

    # Array3Meta
    def Fps(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(28))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Float32Flags, o + self._tab.Pos)
        return 0.0

    # Array3Meta
    def Date(self) -> Optional[str]:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(30))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def Comment(self) -> Optional[str]:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(32))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def Metadata(self, j: int) -> Optional[DictEntry]:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(34))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            obj = DictEntry()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # Array3Meta
    def MetadataLength(self) -> int:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(34))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # Array3Meta
    def MetadataIsNone(self) -> bool:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(34))
        return o == 0

    # Array3Meta
    def Nc(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(36))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

def Array3MetaStart(builder: flatbuffers.Builder):
    builder.StartObject(17)

def Start(builder: flatbuffers.Builder):
    Array3MetaStart(builder)

def Array3MetaAddType(builder: flatbuffers.Builder, type: int):
    builder.PrependInt32Slot(0, type, 0)

def AddType(builder: flatbuffers.Builder, type: int):
    Array3MetaAddType(builder, type)

def Array3MetaAddNx(builder: flatbuffers.Builder, nx: int):
    builder.PrependInt32Slot(1, nx, 0)

def AddNx(builder: flatbuffers.Builder, nx: int):
    Array3MetaAddNx(builder, nx)

def Array3MetaAddNy(builder: flatbuffers.Builder, ny: int):
    builder.PrependInt32Slot(2, ny, 0)

def AddNy(builder: flatbuffers.Builder, ny: int):
    Array3MetaAddNy(builder, ny)

def Array3MetaAddNt(builder: flatbuffers.Builder, nt: int):
    builder.PrependInt32Slot(3, nt, 0)

def AddNt(builder: flatbuffers.Builder, nt: int):
    Array3MetaAddNt(builder, nt)

def Array3MetaAddBitrange(builder: flatbuffers.Builder, bitrange: int):
    builder.PrependInt32Slot(4, bitrange, 0)

def AddBitrange(builder: flatbuffers.Builder, bitrange: int):
    Array3MetaAddBitrange(builder, bitrange)

def Array3MetaAddCmap(builder: flatbuffers.Builder, cmap: int):
    builder.PrependInt32Slot(5, cmap, 0)

def AddCmap(builder: flatbuffers.Builder, cmap: int):
    Array3MetaAddCmap(builder, cmap)

def Array3MetaAddVmin(builder: flatbuffers.Builder, vmin: float):
    builder.PrependFloat32Slot(6, vmin, 0.0)

def AddVmin(builder: flatbuffers.Builder, vmin: float):
    Array3MetaAddVmin(builder, vmin)

def Array3MetaAddVmax(builder: flatbuffers.Builder, vmax: float):
    builder.PrependFloat32Slot(7, vmax, 0.0)

def AddVmax(builder: flatbuffers.Builder, vmax: float):
    Array3MetaAddVmax(builder, vmax)

def Array3MetaAddOpacity(builder: flatbuffers.Builder, opacity: int):
    builder.PrependInt32Slot(8, opacity, 0)

def AddOpacity(builder: flatbuffers.Builder, opacity: int):
    Array3MetaAddOpacity(builder, opacity)

def Array3MetaAddName(builder: flatbuffers.Builder, name: int):
    builder.PrependUOffsetTRelativeSlot(9, flatbuffers.number_types.UOffsetTFlags.py_type(name), 0)

def AddName(builder: flatbuffers.Builder, name: int):
    Array3MetaAddName(builder, name)

def Array3MetaAddParentName(builder: flatbuffers.Builder, parentName: int):
    builder.PrependUOffsetTRelativeSlot(10, flatbuffers.number_types.UOffsetTFlags.py_type(parentName), 0)

def AddParentName(builder: flatbuffers.Builder, parentName: int):
    Array3MetaAddParentName(builder, parentName)

def Array3MetaAddDuration(builder: flatbuffers.Builder, duration: float):
    builder.PrependFloat32Slot(11, duration, 0.0)

def AddDuration(builder: flatbuffers.Builder, duration: float):
    Array3MetaAddDuration(builder, duration)

def Array3MetaAddFps(builder: flatbuffers.Builder, fps: float):
    builder.PrependFloat32Slot(12, fps, 0.0)

def AddFps(builder: flatbuffers.Builder, fps: float):
    Array3MetaAddFps(builder, fps)

def Array3MetaAddDate(builder: flatbuffers.Builder, date: int):
    builder.PrependUOffsetTRelativeSlot(13, flatbuffers.number_types.UOffsetTFlags.py_type(date), 0)

def AddDate(builder: flatbuffers.Builder, date: int):
    Array3MetaAddDate(builder, date)

def Array3MetaAddComment(builder: flatbuffers.Builder, comment: int):
    builder.PrependUOffsetTRelativeSlot(14, flatbuffers.number_types.UOffsetTFlags.py_type(comment), 0)

def AddComment(builder: flatbuffers.Builder, comment: int):
    Array3MetaAddComment(builder, comment)

def Array3MetaAddMetadata(builder: flatbuffers.Builder, metadata: int):
    builder.PrependUOffsetTRelativeSlot(15, flatbuffers.number_types.UOffsetTFlags.py_type(metadata), 0)

def AddMetadata(builder: flatbuffers.Builder, metadata: int):
    Array3MetaAddMetadata(builder, metadata)

def Array3MetaStartMetadataVector(builder, numElems: int) -> int:
    return builder.StartVector(4, numElems, 4)

def StartMetadataVector(builder, numElems: int) -> int:
    return Array3MetaStartMetadataVector(builder, numElems)

def Array3MetaAddNc(builder: flatbuffers.Builder, nc: int):
    builder.PrependInt32Slot(16, nc, 0)

def AddNc(builder: flatbuffers.Builder, nc: int):
    Array3MetaAddNc(builder, nc)

def Array3MetaEnd(builder: flatbuffers.Builder) -> int:
    return builder.EndObject()

def End(builder: flatbuffers.Builder) -> int:
    return Array3MetaEnd(builder)
