# /// script
# dependencies = [
#   "monochrome",  # DO NOT MODIFY THIS LINE
#   "numpy",
# ]
# ///

import sys
import warnings
from datetime import datetime
from pathlib import Path

import monochrome as mc
import numpy as np

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])   # DO NOT MODIFY THIS LINE
print(f"Loading MiCAM recording from '{filepath}' ...")

# Importer classes copied from optimap (https://github.com/cardiacvision/optimap)
class MiCAM05_Importer:
    """Importer for SciMedia MiCAM05 CMOS camera recordings (.gsh / .gsd files).

    .. warning:: Tested only on sample MiCAM05-N256 camera files with 256x256 pixels resolution.
    """

    _Nx, _Ny, _Nt = 0, 0, 0
    _dtype = np.uint16
    _gsd_header_size = 970
    _meta = {}

    def __init__(self, filepath):
        filepath = Path(filepath)
        self.gsd = filepath.with_suffix(".gsd")
        self.gsh = filepath.with_suffix(".gsh")
        if not self.gsd.exists():
            msg = f"File {self.gsd} not found"
            raise FileNotFoundError(msg)
        if not self.gsh.exists():
            msg = f"File {self.gsh} not found"
            raise FileNotFoundError(msg)
        self._read_header()

    def _read_header(self):
        with open(self.gsh, "rt") as fidHeader:
            header = fidHeader.read()
        header = header.split("\n")
        header = [line.split(":", 1) for line in header]
        self._meta = {}
        for line in header:
            if len(line) == 2 and line[1].strip() != "":
                self._meta[line[0].strip()] = line[1].strip()

        self._Nt = int(self._meta["Number of frames"])
        self._Nx = int(self._meta["Image width"])
        self._Ny = int(self._meta["Image height"])
        self._meta["framerate"] = float(self._meta["Frame rate (Hz)"])
        try:
            self._meta["date"] = datetime.strptime(self._meta["Date created"], "%d/%m/%Y %H:%M:%S")
        except ValueError:
            self._meta["date"] = self._meta["Date created"]

    def load_video(self, start_frame=0, frames=None, step=1):
        """Returns a 3D numpy array containing the loaded video."""
        if frames is not None:
            end_frame = start_frame + frames
            if end_frame > self._Nt:
                warnings.warn(f"Requested {frames} frames, but only {self._Nt - start_frame} frames available. "
                              "Loading all available frames.", UserWarning)
                end_frame = None
        else:
            end_frame = None

        background_image = np.fromfile(
                self.gsd,
                dtype=self._dtype,
                count=(self._Nx * self._Ny),
                offset=self._gsd_header_size
            )
        background_image = background_image.reshape((self._Nx, self._Ny))

        CMOS_data = np.memmap(self.gsd,
                              dtype=self._dtype,
                              offset=self._gsd_header_size + background_image.nbytes,
                              shape=(self._Nt, self._Nx, self._Ny),
                              mode="r")
        CMOS_data = CMOS_data[start_frame:end_frame:step].copy()
        return CMOS_data + background_image[np.newaxis, :, :]

    def get_metadata(self):
        """Returns the metadata dictionary."""
        return self._meta

class MiCAM_ULTIMA_Importer:
    """Importer for SciMedia MiCAM ULTIMA recordings (.rsh / .rsm / .rsd files)."""

    _Nx, _Ny, _Nt = 0, 0, 0
    _lskp = 0  # left skip
    _rskp = 0  # right skip
    _blk = 0  # frames per block
    _dtype = np.uint16
    _meta = {}

    def __init__(self, filepath):
        filepath = Path(filepath)
        self.rsh = filepath.with_suffix(".rsh")
        self.rsm = filepath.with_suffix(".rsm")
        self.rsd_list = []
        if not self.rsh.exists():
            msg = f"File {self.rsh} not found"
            raise FileNotFoundError(msg)
        self._read_header()

    def _read_header(self):
        with open(self.rsh, "rt") as fidHeader:
            header = fidHeader.read()
        header = header.split("\n")

        self._parse_topheader(header[1:3])
        idx = header.index("Data-File-List")
        self._parse_metadata(header[3:idx])
        self._parse_filelist(header[idx+1:])

    def _parse_topheader(self, header):
        data = {}
        for s in "".join(header).split("/"):
            if "=" in s:
                key, value = s.split("=", 1)
                data[key] = value
        self._Ny = int(data["x"])
        self._Nx = int(data["y"])
        self._lskp = int(data["lskp"])
        self._rskp = -int(data["rskp"])
        if self._rskp == 0:
            self._rskp = None
        self._blk = int(data["blk"])

    def _parse_metadata(self, header):
        for line in header:
            if "=" in line:
                key, value = line.split("=", 1)
                self._meta[key] = value
        if "page_frames" in self._meta:
            self._Nt = int(self._meta["page_frames"])
        if "acquisition_date" in self._meta:
            try:
                self._meta["date"] = datetime.strptime(self._meta["acquisition_date"], "%Y/%m/%d %H:%M:%S")
            except ValueError:
                self._meta["date"] = self._meta["acquisition_date"]

    def _parse_filelist(self, header):
        base = self.rsh.parent
        for line in header:
            file = base / line.strip()
            if file.suffix == ".rsm":
                self.rsm = file
            elif file.suffix == ".rsd":
                self.rsd_list.append(file)

    def load_video(self, start_frame=0, frames=None, step=1):
        """Returns a 3D numpy array containing the loaded video."""
        if frames is not None:
            end_frame = start_frame + frames
            if end_frame > self._Nt:
                warnings.warn(f"Requested {frames} frames, but only {self._Nt - start_frame} frames available. "
                              "Loading all available frames.", UserWarning)
                end_frame = None
        else:
            end_frame = None

        background = np.fromfile(self.rsm, dtype="<u2")
        background = background.reshape((self._Nx, self._Ny))[np.newaxis, :, self._lskp:self._rskp]

        imgs = None
        for file in self.rsd_list:
            img = np.fromfile(file, dtype="<i2")
            img = img.reshape(self._blk, self._Nx, self._Ny)[..., self._lskp:self._rskp]
            img = background + img
            img = img.astype(np.uint16)
            if imgs is None:
                imgs = img
            else:
                imgs = np.concatenate([imgs, img])
        return imgs[start_frame:end_frame:step]

    def get_metadata(self):
        """Returns the metadata dictionary."""
        return self._meta

suffix = filepath.suffix.lower()
if suffix in [".gsd", ".gsh"]:
    importer = MiCAM05_Importer(filepath)
elif suffix in [".rsh", ".rsm", ".rsd"]:
    importer = MiCAM_ULTIMA_Importer(filepath)
else:
    raise ValueError(f"File suffix '{suffix}' not supported")

video = importer.load_video()
metadata = importer.get_metadata()
metadata["filepath"] = str(filepath)
mc.show_video(video, name=filepath.name, metadata=metadata)
