# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class Array3Meta(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Array3Meta()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsArray3Meta(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Array3Meta
    def Init(self, buf, pos):
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
    def Name(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(22))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def ParentName(self):
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
    def Date(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(30))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def Comment(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(32))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def Metadata(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(34))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from fbs.DictEntry import DictEntry
            obj = DictEntry()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # Array3Meta
    def MetadataLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(34))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # Array3Meta
    def MetadataIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(34))
        return o == 0

def Array3MetaStart(builder):
    builder.StartObject(16)

def Start(builder):
    Array3MetaStart(builder)

def Array3MetaAddType(builder, type):
    builder.PrependInt32Slot(0, type, 0)

def AddType(builder, type):
    Array3MetaAddType(builder, type)

def Array3MetaAddNx(builder, nx):
    builder.PrependInt32Slot(1, nx, 0)

def AddNx(builder, nx):
    Array3MetaAddNx(builder, nx)

def Array3MetaAddNy(builder, ny):
    builder.PrependInt32Slot(2, ny, 0)

def AddNy(builder, ny):
    Array3MetaAddNy(builder, ny)

def Array3MetaAddNt(builder, nt):
    builder.PrependInt32Slot(3, nt, 0)

def AddNt(builder, nt):
    Array3MetaAddNt(builder, nt)

def Array3MetaAddBitrange(builder, bitrange):
    builder.PrependInt32Slot(4, bitrange, 0)

def AddBitrange(builder, bitrange):
    Array3MetaAddBitrange(builder, bitrange)

def Array3MetaAddCmap(builder, cmap):
    builder.PrependInt32Slot(5, cmap, 0)

def AddCmap(builder, cmap):
    Array3MetaAddCmap(builder, cmap)

def Array3MetaAddVmin(builder, vmin):
    builder.PrependFloat32Slot(6, vmin, 0.0)

def AddVmin(builder, vmin):
    Array3MetaAddVmin(builder, vmin)

def Array3MetaAddVmax(builder, vmax):
    builder.PrependFloat32Slot(7, vmax, 0.0)

def AddVmax(builder, vmax):
    Array3MetaAddVmax(builder, vmax)

def Array3MetaAddOpacity(builder, opacity):
    builder.PrependInt32Slot(8, opacity, 0)

def AddOpacity(builder, opacity):
    Array3MetaAddOpacity(builder, opacity)

def Array3MetaAddName(builder, name):
    builder.PrependUOffsetTRelativeSlot(9, flatbuffers.number_types.UOffsetTFlags.py_type(name), 0)

def AddName(builder, name):
    Array3MetaAddName(builder, name)

def Array3MetaAddParentName(builder, parentName):
    builder.PrependUOffsetTRelativeSlot(10, flatbuffers.number_types.UOffsetTFlags.py_type(parentName), 0)

def AddParentName(builder, parentName):
    Array3MetaAddParentName(builder, parentName)

def Array3MetaAddDuration(builder, duration):
    builder.PrependFloat32Slot(11, duration, 0.0)

def AddDuration(builder, duration):
    Array3MetaAddDuration(builder, duration)

def Array3MetaAddFps(builder, fps):
    builder.PrependFloat32Slot(12, fps, 0.0)

def AddFps(builder, fps):
    Array3MetaAddFps(builder, fps)

def Array3MetaAddDate(builder, date):
    builder.PrependUOffsetTRelativeSlot(13, flatbuffers.number_types.UOffsetTFlags.py_type(date), 0)

def AddDate(builder, date):
    Array3MetaAddDate(builder, date)

def Array3MetaAddComment(builder, comment):
    builder.PrependUOffsetTRelativeSlot(14, flatbuffers.number_types.UOffsetTFlags.py_type(comment), 0)

def AddComment(builder, comment):
    Array3MetaAddComment(builder, comment)

def Array3MetaAddMetadata(builder, metadata):
    builder.PrependUOffsetTRelativeSlot(15, flatbuffers.number_types.UOffsetTFlags.py_type(metadata), 0)

def AddMetadata(builder, metadata):
    Array3MetaAddMetadata(builder, metadata)

def Array3MetaStartMetadataVector(builder, numElems):
    return builder.StartVector(4, numElems, 4)

def StartMetadataVector(builder, numElems: int) -> int:
    return Array3MetaStartMetadataVector(builder, numElems)

def Array3MetaEnd(builder):
    return builder.EndObject()

def End(builder):
    return Array3MetaEnd(builder)
