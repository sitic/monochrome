# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
from typing import Any
from .RecordingTracePos import RecordingTracePos
from typing import Optional
np = import_numpy()

class ResponseTracePos(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset: int = 0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = ResponseTracePos()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsResponseTracePos(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # ResponseTracePos
    def Init(self, buf: bytes, pos: int):
        self._tab = flatbuffers.table.Table(buf, pos)

    # ResponseTracePos
    def Recordings(self, j: int) -> Optional[RecordingTracePos]:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            obj = RecordingTracePos()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # ResponseTracePos
    def RecordingsLength(self) -> int:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # ResponseTracePos
    def RecordingsIsNone(self) -> bool:
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        return o == 0

def ResponseTracePosStart(builder: flatbuffers.Builder):
    builder.StartObject(1)

def Start(builder: flatbuffers.Builder):
    ResponseTracePosStart(builder)

def ResponseTracePosAddRecordings(builder: flatbuffers.Builder, recordings: int):
    builder.PrependUOffsetTRelativeSlot(0, flatbuffers.number_types.UOffsetTFlags.py_type(recordings), 0)

def AddRecordings(builder: flatbuffers.Builder, recordings: int):
    ResponseTracePosAddRecordings(builder, recordings)

def ResponseTracePosStartRecordingsVector(builder, numElems: int) -> int:
    return builder.StartVector(4, numElems, 4)

def StartRecordingsVector(builder, numElems: int) -> int:
    return ResponseTracePosStartRecordingsVector(builder, numElems)

def ResponseTracePosEnd(builder: flatbuffers.Builder) -> int:
    return builder.EndObject()

def End(builder: flatbuffers.Builder) -> int:
    return ResponseTracePosEnd(builder)
