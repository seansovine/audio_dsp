#!/usr/bin/env python

from pathlib import Path

from pprint import pprint

import wave

THISDIR = Path(__file__).resolve().parent
FILEDIR = THISDIR / ".." / "src" / "examples" / "media"

FILENAME = "Low E.wav"


def print_file_info(file: wave.Wave_read):
    file_info = file.getparams()
    print("File info:\n  ", end = "")
    pprint(file_info)


if __name__ == "__main__":
    file_name = (FILEDIR / FILENAME).resolve()
    file_name = str(file_name.absolute())
    print("Getting WAV info for file: %s" % file_name)

    with wave.open(file_name) as file:
        print_file_info(file)
