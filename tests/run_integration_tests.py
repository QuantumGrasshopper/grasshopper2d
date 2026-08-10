#!/usr/bin/env python3

import pathlib
import subprocess
import sys
import tempfile


class IntegrationTestFailure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise IntegrationTestFailure(message)


def read_configuration(path, expected_count, grid_area):
    try:
        coordinates = [int(line) for line in path.read_text(encoding="utf-8").splitlines()]
    except (OSError, ValueError) as error:
        raise IntegrationTestFailure(f"could not parse {path.name}: {error}") from error

    require(len(coordinates) == expected_count,
            f"{path.name} contains {len(coordinates)} coordinates, expected {expected_count}")
    require(len(set(coordinates)) == expected_count,
            f"{path.name} contains duplicate coordinates")
    require(all(0 <= coordinate < grid_area for coordinate in coordinates),
            f"{path.name} contains an out-of-bounds coordinate")


def main():
    if len(sys.argv) != 2:
        print(f"usage: {pathlib.Path(sys.argv[0]).name} GRASSHOPPER_EXECUTABLE",
              file=sys.stderr)
        return 2

    executable = pathlib.Path(sys.argv[1]).resolve()
    command = [
        str(executable),
        "-N", "4",
        "-gridsize", "8",
        "-d", "0.25",
        "-hours", "1",
        "-steps", "10",
        "-tempsteps", "10",
        "-inittemp", "20",
        "-fintemp", "0.05",
        "-annealsteps", "100",
        "-configoutput", "0",
        "-initconf", "random",
        "-delta", "0",
        "-NNint", "0",
        "-randomseed", "12345",
    ]
    completed = None

    try:
        require(executable.is_file(), f"executable not found: {executable}")

        with tempfile.TemporaryDirectory(prefix="grasshopper2d-integration-") as directory:
            working_directory = pathlib.Path(directory)
            completed = subprocess.run(
                command,
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode == 0,
                    f"grasshopper exited with status {completed.returncode}")

            result_path = working_directory / "result.dat"
            require(result_path.is_file(), "result.dat was not created")
            result_text = result_path.read_text(encoding="utf-8")
            require("Random seed: 12345" in result_text,
                    "result.dat does not report the requested seed")
            require("Finished after 10 steps" in result_text,
                    "result.dat does not report exactly 10 completed steps")

            for filename in ("energies.dat", "temperatures.dat"):
                output_path = working_directory / filename
                require(output_path.is_file(), f"{filename} was not created")
                require(output_path.stat().st_size > 0, f"{filename} is empty")

            for filename in ("initconf.dat", "finconf.dat", "bestconf.dat"):
                configuration_path = working_directory / filename
                require(configuration_path.is_file(), f"{filename} was not created")
                read_configuration(configuration_path, expected_count=4, grid_area=64)

    except (IntegrationTestFailure, OSError, subprocess.SubprocessError) as error:
        print(f"integration smoke test: FAIL: {error}", file=sys.stderr)
        print("command: " + " ".join(command), file=sys.stderr)
        if completed is not None:
            print("stdout:", file=sys.stderr)
            print(completed.stdout, file=sys.stderr)
            print("stderr:", file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
        return 1

    print("integration smoke test: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
