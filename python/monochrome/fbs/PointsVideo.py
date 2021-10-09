# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class PointsVideo(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = PointsVideo()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsPointsVideo(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # PointsVideo
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # PointsVideo
    def Name(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # PointsVideo
    def ParentName(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # PointsVideo
    def PointsData(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.Get(flatbuffers.number_types.Float32Flags, a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 4))
        return 0

    # PointsVideo
    def PointsDataAsNumpy(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return self._tab.GetVectorAsNumpy(flatbuffers.number_types.Float32Flags, o)
        return 0

    # PointsVideo
    def PointsDataLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # PointsVideo
    def PointsDataIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        return o == 0

    # PointsVideo
    def TimeIdxs(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(10))
        if o != 0:
            a = self._tab.Vector(o)
            return self._tab.Get(flatbuffers.number_types.Uint32Flags, a + flatbuffers.number_types.UOffsetTFlags.py_type(j * 4))
        return 0

    # PointsVideo
    def TimeIdxsAsNumpy(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(10))
        if o != 0:
            return self._tab.GetVectorAsNumpy(flatbuffers.number_types.Uint32Flags, o)
        return 0

    # PointsVideo
    def TimeIdxsLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(10))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # PointsVideo
    def TimeIdxsIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(10))
        return o == 0

    # PointsVideo
    def Color(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(12))
        if o != 0:
            x = o + self._tab.Pos
            from fbs.Color import Color
            obj = Color()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

def Start(builder): builder.StartObject(5)
def PointsVideoStart(builder):
    """This method is deprecated. Please switch to Start."""
    return Start(builder)
def AddName(builder, name): builder.PrependUOffsetTRelativeSlot(0, flatbuffers.number_types.UOffsetTFlags.py_type(name), 0)
def PointsVideoAddName(builder, name):
    """This method is deprecated. Please switch to AddName."""
    return AddName(builder, name)
def AddParentName(builder, parentName): builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(parentName), 0)
def PointsVideoAddParentName(builder, parentName):
    """This method is deprecated. Please switch to AddParentName."""
    return AddParentName(builder, parentName)
def AddPointsData(builder, pointsData): builder.PrependUOffsetTRelativeSlot(2, flatbuffers.number_types.UOffsetTFlags.py_type(pointsData), 0)
def PointsVideoAddPointsData(builder, pointsData):
    """This method is deprecated. Please switch to AddPointsData."""
    return AddPointsData(builder, pointsData)
def StartPointsDataVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def PointsVideoStartPointsDataVector(builder, numElems):
    """This method is deprecated. Please switch to Start."""
    return StartPointsDataVector(builder, numElems)
def AddTimeIdxs(builder, timeIdxs): builder.PrependUOffsetTRelativeSlot(3, flatbuffers.number_types.UOffsetTFlags.py_type(timeIdxs), 0)
def PointsVideoAddTimeIdxs(builder, timeIdxs):
    """This method is deprecated. Please switch to AddTimeIdxs."""
    return AddTimeIdxs(builder, timeIdxs)
def StartTimeIdxsVector(builder, numElems): return builder.StartVector(4, numElems, 4)
def PointsVideoStartTimeIdxsVector(builder, numElems):
    """This method is deprecated. Please switch to Start."""
    return StartTimeIdxsVector(builder, numElems)
def AddColor(builder, color): builder.PrependStructSlot(4, flatbuffers.number_types.UOffsetTFlags.py_type(color), 0)
def PointsVideoAddColor(builder, color):
    """This method is deprecated. Please switch to AddColor."""
    return AddColor(builder, color)
def End(builder): return builder.EndObject()
def PointsVideoEnd(builder):
    """This method is deprecated. Please switch to End."""
    return End(builder)