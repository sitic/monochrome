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
    def Name(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(12))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def Duration(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(14))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Float32Flags, o + self._tab.Pos)
        return 0.0

    # Array3Meta
    def Fps(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(16))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Float32Flags, o + self._tab.Pos)
        return 0.0

    # Array3Meta
    def Date(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(18))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def Comment(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(20))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def Bitrange(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(22))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def Cmap(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(24))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def ParentName(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(26))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3Meta
    def AlphaTransferFct(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(28))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3Meta
    def MetaData(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(30))
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
    def MetaDataLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(30))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # Array3Meta
    def MetaDataIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(30))
        return o == 0

def Start(builder): builder.StartObject(14)
def Array3MetaStart(builder):
    """This method is deprecated. Please switch to Start."""
    return Start(builder)
def AddType(builder, type): builder.PrependInt32Slot(0, type, 0)
def Array3MetaAddType(builder, type):
    """This method is deprecated. Please switch to AddType."""
    return AddType(builder, type)
def AddNx(builder, nx): builder.PrependInt32Slot(1, nx, 0)
def Array3MetaAddNx(builder, nx):
    """This method is deprecated. Please switch to AddNx."""
    return AddNx(builder, nx)
def AddNy(builder, ny): builder.PrependInt32Slot(2, ny, 0)
def Array3MetaAddNy(builder, ny):
    """This method is deprecated. Please switch to AddNy."""
    return AddNy(builder, ny)
def AddNt(builder, nt): builder.PrependInt32Slot(3, nt, 0)
def Array3MetaAddNt(builder, nt):
    """This method is deprecated. Please switch to AddNt."""
    return AddNt(builder, nt)
def AddName(builder, name): builder.PrependUOffsetTRelativeSlot(4, flatbuffers.number_types.UOffsetTFlags.py_type(name), 0)
def Array3MetaAddName(builder, name):
    """This method is deprecated. Please switch to AddName."""
    return AddName(builder, name)
def AddDuration(builder, duration): builder.PrependFloat32Slot(5, duration, 0.0)
def Array3MetaAddDuration(builder, duration):
    """This method is deprecated. Please switch to AddDuration."""
    return AddDuration(builder, duration)
def AddFps(builder, fps): builder.PrependFloat32Slot(6, fps, 0.0)
def Array3MetaAddFps(builder, fps):
    """This method is deprecated. Please switch to AddFps."""
    return AddFps(builder, fps)
def AddDate(builder, date): builder.PrependUOffsetTRelativeSlot(7, flatbuffers.number_types.UOffsetTFlags.py_type(date), 0)
def Array3MetaAddDate(builder, date):
    """This method is deprecated. Please switch to AddDate."""
    return AddDate(builder, date)
def AddComment(builder, comment): builder.PrependUOffsetTRelativeSlot(8, flatbuffers.number_types.UOffsetTFlags.py_type(comment), 0)
def Array3MetaAddComment(builder, comment):
    """This method is deprecated. Please switch to AddComment."""
    return AddComment(builder, comment)
def AddBitrange(builder, bitrange): builder.PrependInt32Slot(9, bitrange, 0)
def Array3MetaAddBitrange(builder, bitrange):
    """This method is deprecated. Please switch to AddBitrange."""
    return AddBitrange(builder, bitrange)
def AddCmap(builder, cmap): builder.PrependInt32Slot(10, cmap, 0)
def Array3MetaAddCmap(builder, cmap):
    """This method is deprecated. Please switch to AddCmap."""
    return AddCmap(builder, cmap)
def AddParentName(builder, parentName): builder.PrependUOffsetTRelativeSlot(11, flatbuffers.number_types.UOffsetTFlags.py_type(parentName), 0)
def Array3MetaAddParentName(builder, parentName):
    """This method is deprecated. Please switch to AddParentName."""
    return AddParentName(builder, parentName)
def AddAlphaTransferFct(builder, alphaTransferFct): builder.PrependInt32Slot(12, alphaTransferFct, 0)
def Array3MetaAddAlphaTransferFct(builder, alphaTransferFct):
    """This method is deprecated. Please switch to AddAlphaTransferFct."""
    return AddAlphaTransferFct(builder, alphaTransferFct)
def AddMetaData(builder, metaData): builder.PrependUOffsetTRelativeSlot(13, flatbuffers.number_types.UOffsetTFlags.py_type(metaData), 0)
def Array3MetaAddMetaData(builder, metaData):
    """This method is deprecated. Please switch to AddMetaData."""
    return AddMetaData(builder, metaData)
def StartMetaDataVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def Array3MetaStartMetaDataVector(builder, numElems):
    """This method is deprecated. Please switch to Start."""
    return StartMetaDataVector(builder, numElems)
def End(builder): return builder.EndObject()
def Array3MetaEnd(builder):
    """This method is deprecated. Please switch to End."""
    return End(builder)