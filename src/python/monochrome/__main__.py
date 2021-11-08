import sys

from .ipc import console_entrypoint as _main

if __name__ == "__main__":
    sys.exit(_main())