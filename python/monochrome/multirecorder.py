# Basic MultiRecorder file importer, based on the code from PythonAnalyser

import numpy as np
import struct
from datetime import datetime
import locale
from pathlib import Path

from .utils import show_array, show_layer


class MultRecDatImport():
    def __init__(self, filename, use8BitMode=False, readNFrames=0, skipNFrames=0, cropTop=0, cropLeft=0, cropWidth=0,
                 cropHeight=0, subsK=1, binning_x=1, binning_y=1, binning_z=1, skipHeader=False, manualSx=-1,
                 manualSy=-1, manualNframes=-1, flip_image=False, transpose_image=False, image_orientation="North",
                 memory_map=False):
        """

        :param use8BitMode: boolean, Size of each pixel
        :param readNFrames: integer, Read only N frames
        :param skipNFrames: integer, Skip N frames

        :param cropTop: integer, Crop Y pixels from top
        :param cropLeft: integer, Crop X pixels from left
        :param cropWidth: integer, Crop width
        :param cropHeight: integer, Crop height

        :param subsK: integer, Take only every k-th frame
        :param binning_x: integer, Bin the file in X direction
        :param binning_y: integer, Bin the file in Y direction
        :param binning_z: integer, Bin the file in Z direction

        :param skipHeader: boolean, Skip the header in case of corrupted / not finished dat files.
        :param manualSx: integer, Manual Size X
        :param manualSy: integer, Manual Size Y
        :param manualNframes: integer, Manual N frames

        :param flip_image: boolean, Flips the slow axis of the image (before transposing). (Internally we use the lower origin convention)
        :param transpose_image: boolean, Swaps the x and y coordinates (flipping+rotation could already do this) of the original data. This means the slow (parallel) variable is considered x and the fast (seriel) is considered to be y. This is not identical to transposing the image. (TODO: Frankly this might be a bit wrong)

        :param image_orientation: enum, oneof ["North", "East", "South", "West"]. Where "up" is orientated, note that a x-y flip is Flip image + rotation.
        :param memory_map: boolean, Use memory mapped region, the array will be read only. This means that *no* memory will be used. It is not compatible to binning.
        """
        self.filename = Path(filename)

        self.use8BitMode = use8BitMode
        self.readNFrames = readNFrames
        self.skipNFrames = skipNFrames

        self.cropTop = cropTop
        self.cropLeft = cropLeft
        self.cropWidth = cropWidth
        self.cropHeight = cropHeight

        self.subsK = subsK
        self.binning_x = binning_x
        self.binning_y = binning_y
        self.binning_z = binning_z

        self.skipHeader = skipHeader
        self.manualSx = manualSx
        self.manualSy = manualSy
        self.manualNframes = manualNframes

        self.flip_image = flip_image
        self.transpose_image = transpose_image

        if image_orientation not in ("North", "South", "East", "West"):
            raise ValueError("invalid image orientation.")
        self.image_orientation = image_orientation
        self.memory_map = memory_map

        self.import_file()

    def show(self):
        show_array(self.data, str(self.filename), fps=self.framerate, comment=self.comment, date=str(self.starttime))

    def show_layer(self, arr, name="", **kwargs):
        show_layer(arr, self.filename.name, name=name, **kwargs)

    def bin_array(self, arr, bin_sizes):
        arr = np.asarray(arr)
        if len(bin_sizes) > arr.ndim:
            raise ValueError
        binning = np.ones(arr.ndim, dtype=np.intp)
        binning[-len(bin_sizes):] = bin_sizes

        shape = np.asarray(arr.shape)
        new_shape = np.zeros(arr.ndim * 2, dtype=np.intp)
        new_shape[::2] = shape // binning
        new_shape[1::2] = binning

        if (new_shape[::2] * new_shape[1::2] != shape).any():
            raise ValueError("shapes incompatible.")

        return arr.reshape(new_shape).mean(tuple(range(1, arr.ndim * 2, 2)))

    def import_file(self, *args, **kwargs):
        with open(self.filename, "rb") as f:
            if not self.skipHeader:
                self.read_header(f)
            else:
                self.sx = self.manualSx
                self.sy = self.manualSy
                self.nframes = self.manualNframes

            if self.cropWidth <= 0:
                self.cropWidth += self.sx - self.cropLeft
            if self.cropHeight <= 0:
                self.cropHeight += self.sy - self.cropTop

            # TODO explain why this is loaded as *signed* int32

            # ---
            if self.readNFrames > 0:
                self.nframes = self.readNFrames
            else:
                self.nframes -= self.skipNFrames
            self.nframes = np.int32(np.floor(self.nframes / self.subsK))

            self.data = np.squeeze(self.read_frames(f))

    def read_frames(self, f):
        if self.use8BitMode:
            dt = np.dtype(self.endianness + "B")
            bs = 1
        else:
            dt = np.dtype(self.endianness + "H")
            bs = 2

        # skip the current part, note the ", 1" which seeks from current pos:
        f.seek((self.sx * self.sy * bs + self.skipAfterEachFrame) *
               self.skipNFrames, 1)

        # Skip before:
        f.seek(self.sy * self.cropTop * bs, 1)
        f.seek(self.cropLeft * bs, 1)

        # Use memory mappping for starters, the array is then copied later.
        # The f.seek(0) may not be necessary, I am not sure, but do anyway.
        offset = f.tell()
        f.seek(0)
        arr = np.memmap(
            f, np.byte, mode='r', offset=offset,
            shape=(self.sx * self.sy * bs +
                   self.skipAfterEachFrame) * self.nframes * self.subsK)
        # import code
        # code.interact(local=locals())
        arr = np.ndarray((self.nframes, self.cropWidth,
                          self.cropHeight, 1), dt,
                         arr,  # Evtl. None
                         0,
                         ((bs * self.sx * self.sy +
                           self.skipAfterEachFrame) * self.subsK,
                          self.sy * bs, bs, 1))

        if self.flip_image:
            # Flip the slow axis of the image.
            arr = arr[:, ::-1, :, :]

        if self.transpose_image:
            # If the image is not transpose, x and y need to be swapped,
            # since up until now x is the slow variable and we assume it is
            # the fast one.
            arr = arr.transpose(0, 2, 1, 3)

        if self.image_orientation == "North":
            pass
        elif self.image_orientation == "East":
            arr = arr[:, :, ::-1, :].transpose(0, 2, 1, 3)
        elif self.image_orientation == "South":
            arr = arr[:, ::-1, ::-1, :]
        elif self.image_orientation == "West":
            arr = arr.transpose(0, 2, 1, 3)[:, :, ::-1, :]
        else:
            raise ValueError("invalid image orientation.")

        # And now, we still need to transpose the y-axis, since we assume
        # the input is in CS format (origin is top) but internally we use
        # origin in bottom format.
        # arr = arr[:, :, ::-1, :]

        if self.binning_z != 1 or self.binning_x != 1 or self.binning_y != 1:
            arr = self.bin_array(arr,
                                 (self.binning_z,
                                  self.binning_x,
                                  self.binning_y, 1))
        elif not self.memory_map:
            arr = arr.copy("C")

        return arr

    def read_header(self, f):
        """
        Read the header.

        f is a file object.
        """
        self.version = f.read(1).decode('utf-8')
        self.endianness = "<"
        self.skipAfterEachFrame = 0
        sum = 1
        if self.version == "d":
            self.date = f.read(17)
            if self.date[-3:-2] != ":":
                self.date += f.read(7)
                sum += 7
            self.nframes = struct.unpack(self.endianness + "i", f.read(4))[0]
            self.sy = struct.unpack(self.endianness + "i", f.read(4))[0]
            self.sx = struct.unpack(self.endianness + "i", f.read(4))[0]

            f.read(10)  # Skip ...
            sum += 17 + 3 * 4 + 10
            f.read(1024 - sum)

        elif self.version == "e" or self.version == "f":
            en = struct.unpack(">i", f.read(4))[0]
            if en == 439041101:
                self.endianness = ">"
            else:
                self.endianness = "<"
            self.nframes = struct.unpack(self.endianness + "i", f.read(4))[0]
            self.sy = struct.unpack(self.endianness + "i", f.read(4))[0]
            self.sx = struct.unpack(self.endianness + "i", f.read(4))[0]
            f.read(8)
            self.framerate = struct.unpack(self.endianness + "i",
                                           f.read(4))[0] / 100000

            dtime = f.read(24).decode('utf-8').rstrip('\x00')

            # Only works with US local:
            cl = locale.getlocale(locale.LC_TIME)
            locale.setlocale(locale.LC_TIME, (None, None))
            try:
                self.starttime = datetime.strptime(dtime, "%c")
            except ValueError:
                try:
                    dtime2 = ":".join(dtime.split(":")[:-1])
                    self.starttime = datetime.strptime(dtime2, "%Y-%m-%d %H:%M:%S")
                except ValueError:
                    self.starttime = datetime.fromtimestamp(0)
                    print("Could not determine video starttime from string: " + str(dtime))

            locale.setlocale(locale.LC_TIME, cl)
            self.comment = f.read(971).rstrip(b'\x00').decode('utf-8')
            self.skipAfterEachFrame = 8
