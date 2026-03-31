from os import listdir
from shutil import move


def find(vec, pred, start=None, end=None):
    start = 0 if start is None else start
    end = len(vec) if end is None else end
    for i in range(start, end):
        if pred(vec[i]):
            return i


def rfind(vec, pred, start=None, end=None):
    start = 0 if start is None else start
    end = len(vec) if end is None else end
    for i in range(end - 1, start - 1, -1):
        if pred(vec[i]):
            return i


def empty_line(line):
    return len(line.strip()) == 0


def prepend(file, line):
    with open(file, "r") as f:
        lines = f.readlines()
    with open(file, "w") as f:
        print(line, file=f)
        print(file=f)
        f.writelines(lines)


def main():
    header_files = [f for f in listdir(".") if f.endswith(".h")]

    def include(*thing):
        return lambda line: "#include" in line and any(f'"{h}"' in line for h in thing)

    with open("dwm.c", "r") as f:
        lines = f.readlines()

    l = {}

    l["any.h"] = find(lines, include(*header_files))
    l["0-extlib"] = rfind(lines, empty_line, end=l["any.h"])

    l["config.h"] = find(lines, include("config.h"))
    l["0-typedef_end"] = rfind(lines, empty_line, end=l["config.h"])

    l["#include"] = rfind(lines, include(), end=l["config.h"])
    l["0-typedef_start"] = find(lines, empty_line, start=l["#include"])

    assert l["0-typedef_start"] is not None
    assert l["0-typedef_end"] is not None
    assert l["0-extlib"] is not None

    # Create a "prelude.h" that contains all external library includes.
    with open("prelude.h", "w") as f:
        f.writelines(lines[: l["0-extlib"]])

    # Create a "dtypes.h" that contains all the types.
    with open("dtypes.h", "w") as f:
        print('#include "prelude.h"', file=f)
        f.writelines(lines[l["0-typedef_start"] + 1 : l["0-typedef_end"]])

    # Update the main "dwm.c".
    with open("dwm.c", "w") as f:
        f.writelines(lines[l["0-typedef_end"] + 1 :])

    move("config.def.h", "config.h")
    prepend("config.h", '#include "dtypes.h"')
    prepend("drw.h", '#include "prelude.h"')
    prepend("util.h", '#include "prelude.h"')


if __name__ == "__main__":
    main()
