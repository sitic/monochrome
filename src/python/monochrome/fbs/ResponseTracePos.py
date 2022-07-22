# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class ResponseTracePos(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = ResponseTracePos()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsResponseTracePos(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # ResponseTracePos
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # ResponseTracePos
    def Recordings(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from fbs.RecordingTracePos import RecordingTracePos
            obj = RecordingTracePos()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # ResponseTracePos
    def RecordingsLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # ResponseTracePos
    def RecordingsIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        return o == 0

def Start(builder): builder.StartObject(1)
def ResponseTracePosStart(builder):
    """This method is deprecated. Please switch to Start."""
    return Start(builder)
def AddRecordings(builder, recordings): builder.PrependUOffsetTRelativeSlot(0, flatbuffers.number_types.UOffsetTFlags.py_type(recordings), 0)
def ResponseTracePosAddRecordings(builder, recordings):
    """This method is deprecated. Please switch to AddRecordings."""
    return AddRecordings(builder, recordings)
def StartRecordingsVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def ResponseTracePosStartRecordingsVector(builder, numElems):
    """This method is deprecated. Please switch to Start."""
    return StartRecordingsVector(builder, numElems)
def End(builder): return builder.EndObject()
def ResponseTracePosEnd(builder):
    """This method is deprecated. Please switch to End."""
    return End(builder)