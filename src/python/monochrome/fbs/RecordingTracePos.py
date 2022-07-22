# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class RecordingTracePos(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = RecordingTracePos()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsRecordingTracePos(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # RecordingTracePos
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # RecordingTracePos
    def Name(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # RecordingTracePos
    def Posx(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.Get(flatbuffers.number_types.Uint32Flags, a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 4))
        return 0

    # RecordingTracePos
    def PosxAsNumpy(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.GetVectorAsNumpy(flatbuffers.number_types.Uint32Flags, o)
        return 0

    # RecordingTracePos
    def PosxLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # RecordingTracePos
    def PosxIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        return o == 0

    # RecordingTracePos
    def Posy(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.Get(flatbuffers.number_types.Uint32Flags, a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 4))
        return 0

    # RecordingTracePos
    def PosyAsNumpy(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return self._tab.GetVectorAsNumpy(flatbuffers.number_types.Uint32Flags, o)
        return 0

    # RecordingTracePos
    def PosyLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # RecordingTracePos
    def PosyIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        return o == 0

def Start(builder): builder.StartObject(3)
def RecordingTracePosStart(builder):
    """This method is deprecated. Please switch to Start."""
    return Start(builder)
def AddName(builder, name): builder.PrependUOffsetTRelativeSlot(0, flatbuffers.number_types.UOffsetTFlags.py_type(name), 0)
def RecordingTracePosAddName(builder, name):
    """This method is deprecated. Please switch to AddName."""
    return AddName(builder, name)
def AddPosx(builder, posx): builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(posx), 0)
def RecordingTracePosAddPosx(builder, posx):
    """This method is deprecated. Please switch to AddPosx."""
    return AddPosx(builder, posx)
def StartPosxVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def RecordingTracePosStartPosxVector(builder, numElems):
    """This method is deprecated. Please switch to Start."""
    return StartPosxVector(builder, numElems)
def AddPosy(builder, posy): builder.PrependUOffsetTRelativeSlot(2, flatbuffers.number_types.UOffsetTFlags.py_type(posy), 0)
def RecordingTracePosAddPosy(builder, posy):
    """This method is deprecated. Please switch to AddPosy."""
    return AddPosy(builder, posy)
def StartPosyVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def RecordingTracePosStartPosyVector(builder, numElems):
    """This method is deprecated. Please switch to Start."""
    return StartPosyVector(builder, numElems)
def End(builder): return builder.EndObject()
def RecordingTracePosEnd(builder):
    """This method is deprecated. Please switch to End."""
    return End(builder)