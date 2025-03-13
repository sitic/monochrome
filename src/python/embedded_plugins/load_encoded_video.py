# /// script
# dependencies = [
#   "monochrome",  # DO NOT MODIFY THIS LINE
#   "scikit-video @ https://github.com/scikit-video/scikit-video/archive/refs/heads/master.zip",
#   "static_ffmpeg",
# ]
# ///
import sys
from pathlib import Path

import monochrome as mc
import skvideo
import skvideo.io
import static_ffmpeg.run

# filepath will be set by Monochrome when the script is run
filepath = Path(sys.argv[1])   # DO NOT MODIFY THIS LINE
print(f"Loading video from '{filepath}' ...")

def _fix_ffmpeg_location():
    """Make skvideo use static ffmpeg if system ffmpeg not found."""
    if not skvideo._HAS_FFMPEG:
        print("Could not find ffmpeg in PATH. Downloading static ffmpeg binary")
        # ffmpeg not found, download static binary for it
        ffmpeg, _  = static_ffmpeg.run.get_or_fetch_platform_executables_else_raise()
        skvideo.setFFmpegPath(str(Path(ffmpeg).parent))
        assert skvideo._HAS_FFMPEG

        # fix global variables
        skvideo.io.ffmpeg._HAS_FFMPEG = skvideo._HAS_FFMPEG
        skvideo.io.ffmpeg._FFMPEG_PATH = skvideo._FFMPEG_PATH
        skvideo.io.ffmpeg._FFMPEG_SUPPORTED_DECODERS = skvideo._FFMPEG_SUPPORTED_DECODERS
        skvideo.io.ffmpeg._FFMPEG_SUPPORTED_ENCODERS = skvideo._FFMPEG_SUPPORTED_ENCODERS

def load_encoded_video(filename, as_grey=True):
    _fix_ffmpeg_location()
    data = skvideo.io.vread(
        str(filename),
        as_grey=as_grey
    )
    if as_grey:
        data = data[..., 0]
    return data

video = load_encoded_video(filepath, as_grey=False)
metadata = {
    "filepath": str(filepath),
    "shape": str(video.shape),
    "dtype": str(video.dtype),
}
mc.show_video(video, name=filepath.name, metadata=metadata)
