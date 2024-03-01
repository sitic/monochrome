# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
from typing import Any
np = import_numpy()

class Filepaths(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset: int = 0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Filepaths()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsFilepaths(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Filepaths
    def Init(self, buf: bytes, pos: int):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Filepaths
    def File(self, j: int):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.String(a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 4))
        return ""

    # Filepaths
    def FileLength(self) -> int:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # Filepaths
    def FileIsNone(self) -> bool:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        return o == 0

def FilepathsStart(builder: flatbuffers.Builder):
    builder.StartObject(1)

def Start(builder: flatbuffers.Builder):
    FilepathsStart(builder)

def FilepathsAddFile(builder: flatbuffers.Builder, file: int):
    builder.PrependUOffsetTRelativeSlot(0, flatbuffers.number_types.UOffsetTFlags.py_type(file), 0)

def AddFile(builder: flatbuffers.Builder, file: int):
    FilepathsAddFile(builder, file)

def FilepathsStartFileVector(builder, numElems: int) -> int:
    return builder.StartVector(4, numElems, 4)

def StartFileVector(builder, numElems: int) -> int:
    return FilepathsStartFileVector(builder, numElems)

def FilepathsEnd(builder: flatbuffers.Builder) -> int:
    return builder.EndObject()

def End(builder: flatbuffers.Builder) -> int:
    return FilepathsEnd(builder)
