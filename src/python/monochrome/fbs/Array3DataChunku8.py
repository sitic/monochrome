# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
from typing import Any
np = import_numpy()

class Array3DataChunku8(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset: int = 0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Array3DataChunku8()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsArray3DataChunku8(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Array3DataChunku8
    def Init(self, buf: bytes, pos: int):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Array3DataChunku8
    def Startidx(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint64Flags, o + self._tab.Pos)
        return 0

    # Array3DataChunku8
    def Data(self, j: int):
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
    def DataLength(self) -> int:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # Array3DataChunku8
    def DataIsNone(self) -> bool:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        return o == 0

def Array3DataChunku8Start(builder: flatbuffers.Builder):
    builder.StartObject(2)

def Start(builder: flatbuffers.Builder):
    Array3DataChunku8Start(builder)

def Array3DataChunku8AddStartidx(builder: flatbuffers.Builder, startidx: int):
    builder.PrependUint64Slot(0, startidx, 0)

def AddStartidx(builder: flatbuffers.Builder, startidx: int):
    Array3DataChunku8AddStartidx(builder, startidx)

def Array3DataChunku8AddData(builder: flatbuffers.Builder, data: int):
    builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(data), 0)

def AddData(builder: flatbuffers.Builder, data: int):
    Array3DataChunku8AddData(builder, data)

def Array3DataChunku8StartDataVector(builder, numElems: int) -> int:
    return builder.StartVector(1, numElems, 1)

def StartDataVector(builder, numElems: int) -> int:
    return Array3DataChunku8StartDataVector(builder, numElems)

def Array3DataChunku8End(builder: flatbuffers.Builder) -> int:
    return builder.EndObject()

def End(builder: flatbuffers.Builder) -> int:
    return Array3DataChunku8End(builder)
