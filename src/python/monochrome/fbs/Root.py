# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class Root(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Root()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsRoot(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Root
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Root
    def DataType(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint8Flags, o + self._tab.Pos)
        return 0

    # Root
    def Data(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            from flatbuffers.table import Table
            obj = Table(bytearray(), 0)
            self._tab.Union(obj, o)
            return obj
        return None

def RootStart(builder): builder.StartObject(2)
def Start(builder):
    return RootStart(builder)
def RootAddDataType(builder, dataType): builder.PrependUint8Slot(0, dataType, 0)
def AddDataType(builder, dataType):
    return RootAddDataType(builder, dataType)
def RootAddData(builder, data): builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(data), 0)
def AddData(builder, data):
    return RootAddData(builder, data)
def RootEnd(builder): return builder.EndObject()
def End(builder):
    return RootEnd(builder)