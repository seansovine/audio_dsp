#!/usr/bin/env python

from pathlib import Path
from pprint import pprint

import wave

IS_OUTPUT = False

THISDIR = Path(__file__).resolve().parent
FILEDIR = THISDIR / ".." / "media"

if IS_OUTPUT:
    FILEDIR = FILEDIR / "output"

FILENAME = "Low E.wav"


def print_file_info(file: wave.Wave_read):
    file_info = file.getparams()
    print("  File info:\n    ", end="")
    pprint(file_info)


if __name__ == "__main__":
    ITERDIR = THISDIR / "../scratch"  # = FILEDIR
    # file_name = (FILEDIR / FILENAME).resolve()
    # file_name = Path(THISDIR / "../scratch/In Flames - Meet Your Maker [dNCPltNOWM0].wav").resolve()
    for file_name in Path(ITERDIR).iterdir():
        if not file_name.is_file() or file_name.suffix != ".wav":
            continue

        file_name = str(file_name.absolute())
        print("Getting WAV info for file: %s" % file_name)

        with wave.open(file_name) as file:
            print_file_info(file)
