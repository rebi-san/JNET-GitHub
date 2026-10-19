from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
WORK = HERE.parent / "work/network-0.142"


def main():
    WORK.mkdir(exist_ok=True)
    source = (HERE / "traveler_network.h").read_text()
    first = source.index("static void traveler_network_slot_close")
    last = source.index("static void traveler_network_send")
    (WORK / "network_fixture.h").write_text(source[:first] + source[last:])
    for name in ("jnet_menu_model", "companion_connection", "traveler_network"):
        target = WORK / ("test_" + name)
        subprocess.run(["clang", "-arch", "x86_64", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-I", str(HERE), "-I", str(WORK),
                        str(HERE / ("test_" + name + ".c")), "-o", str(target)], check=True)
        subprocess.run([str(target)], check=True, timeout=30)


if __name__ == "__main__":
    main()
