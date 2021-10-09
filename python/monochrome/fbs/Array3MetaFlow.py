# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class Array3MetaFlow(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Array3MetaFlow()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsArray3MetaFlow(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Array3MetaFlow
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Array3MetaFlow
    def Nx(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3MetaFlow
    def Ny(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3MetaFlow
    def Nt(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # Array3MetaFlow
    def Name(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(10))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3MetaFlow
    def ParentName(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(12))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Array3MetaFlow
    def Color(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(14))
        if o != 0:
            x = o + self._tab.Pos
            from fbs.Color import Color
            obj = Color()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

def Start(builder): builder.StartObject(6)
def Array3MetaFlowStart(builder):
    """This method is deprecated. Please switch to Start."""
    return Start(builder)
def AddNx(builder, nx): builder.PrependInt32Slot(0, nx, 0)
def Array3MetaFlowAddNx(builder, nx):
    """This method is deprecated. Please switch to AddNx."""
    return AddNx(builder, nx)
def AddNy(builder, ny): builder.PrependInt32Slot(1, ny, 0)
def Array3MetaFlowAddNy(builder, ny):
    """This method is deprecated. Please switch to AddNy."""
    return AddNy(builder, ny)
def AddNt(builder, nt): builder.PrependInt32Slot(2, nt, 0)
def Array3MetaFlowAddNt(builder, nt):
    """This method is deprecated. Please switch to AddNt."""
    return AddNt(builder, nt)
def AddName(builder, name): builder.PrependUOffsetTRelativeSlot(3, flatbuffers.number_types.UOffsetTFlags.py_type(name), 0)
def Array3MetaFlowAddName(builder, name):
    """This method is deprecated. Please switch to AddName."""
    return AddName(builder, name)
def AddParentName(builder, parentName): builder.PrependUOffsetTRelativeSlot(4, flatbuffers.number_types.UOffsetTFlags.py_type(parentName), 0)
def Array3MetaFlowAddParentName(builder, parentName):
    """This method is deprecated. Please switch to AddParentName."""
    return AddParentName(builder, parentName)
def AddColor(builder, color): builder.PrependStructSlot(5, flatbuffers.number_types.UOffsetTFlags.py_type(color), 0)
def Array3MetaFlowAddColor(builder, color):
    """This method is deprecated. Please switch to AddColor."""
    return AddColor(builder, color)
def End(builder): return builder.EndObject()
def Array3MetaFlowEnd(builder):
    """This method is deprecated. Please switch to End."""
    return End(builder)