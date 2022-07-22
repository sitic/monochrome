# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class Array3DataChunku8(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Array3DataChunku8()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsArray3DataChunku8(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Array3DataChunku8
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Array3DataChunku8
    def Startidx(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint64Flags, o + self._tab.Pos)
        return 0

    # Array3DataChunku8
    def Data(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.Get(flatbuffers.number_types.Uint8Flags, a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 1))
        return 0

    # Array3DataChunku8
    def DataAsNumpy(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.GetVectorAsNumpy(flatbuffers.number_types.Uint8Flags, o)
        return 0

    # Array3DataChunku8
    def DataLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # Array3DataChunku8
    def DataIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        return o == 0

def Start(builder): builder.StartObject(2)
def Array3DataChunku8Start(builder):
    """This method is deprecated. Please switch to Start."""
    return Start(builder)
def AddStartidx(builder, startidx): builder.PrependUint64Slot(0, startidx, 0)
def Array3DataChunku8AddStartidx(builder, startidx):
    """This method is deprecated. Please switch to AddStartidx."""
    return AddStartidx(builder, startidx)
def AddData(builder, data): builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(data), 0)
def Array3DataChunku8AddData(builder, data):
    """This method is deprecated. Please switch to AddData."""
    return AddData(builder, data)
def StartDataVector(builder, numElems): return builder.StartVector(1, numElems, 1)
def Array3DataChunku8StartDataVector(builder, numElems):
    """This method is deprecated. Please switch to Start."""
    return StartDataVector(builder, numElems)
def End(builder): return builder.EndObject()
def Array3DataChunku8End(builder):
    """This method is deprecated. Please switch to End."""
    return End(builder)