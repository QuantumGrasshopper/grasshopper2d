#!/usr/bin/env python3

import math
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


def direct_pairwise_probability(coordinates, grid_size, hopping_distance):
    cell_size = 1.0 / math.sqrt(len(coordinates))
    energy = 0.0

    for first_index, first_coordinate in enumerate(coordinates):
        first_x = first_coordinate % grid_size
        first_y = first_coordinate // grid_size
        for second_coordinate in coordinates[first_index + 1:]:
            second_x = second_coordinate % grid_size
            second_y = second_coordinate // grid_size
            distance = cell_size * math.hypot(first_x - second_x, first_y - second_y)
            normalized_offset = abs(hopping_distance - distance) / cell_size
            if normalized_offset <= 2.0:
                energy += (1.0 + math.cos(math.pi * normalized_offset / 2.0)) / 4.0

    normalization = math.pi * hopping_distance * len(coordinates) ** 1.5
    return energy / normalization


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

        odd_grid_coordinates = [11, 12]
        hopping_distance = 1.5
        command = [
            str(executable),
            "-N", "2",
            "-gridsize", "5",
            "-d", str(hopping_distance),
            "-hours", "1",
            "-steps", "1",
            "-tempsteps", "10",
            "-inittemp", "20",
            "-fintemp", "0.05",
            "-annealsteps", "100",
            "-configoutput", "0",
            "-initconf", "load",
            "-delta", "0",
            "-NNint", "0",
            "-randomseed", "12345",
        ]

        with tempfile.TemporaryDirectory(prefix="grasshopper2d-odd-grid-") as directory:
            working_directory = pathlib.Path(directory)
            (working_directory / "initconf.dat").write_text("11\n12\n", encoding="utf-8")
            completed = subprocess.run(
                command,
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode == 0,
                    f"odd-grid grasshopper run exited with status {completed.returncode}")

            energy_lines = (working_directory / "energies.dat").read_text(
                encoding="utf-8").splitlines()
            require(energy_lines, "odd-grid energies.dat is empty")
            try:
                table_probability = float(energy_lines[0])
            except ValueError as error:
                raise IntegrationTestFailure(
                    "odd-grid energies.dat does not begin with a number") from error

            direct_probability = direct_pairwise_probability(
                odd_grid_coordinates, grid_size=5, hopping_distance=hopping_distance)
            require(math.isclose(table_probability, direct_probability,
                                 rel_tol=0.0, abs_tol=5.1e-7),
                    "odd-grid neighbor-table probability "
                    f"{table_probability:.17g} does not match direct pairwise probability "
                    f"{direct_probability:.17g}")

    except (IntegrationTestFailure, OSError, subprocess.SubprocessError) as error:
        print(f"integration tests: FAIL: {error}", file=sys.stderr)
        print("command: " + " ".join(command), file=sys.stderr)
        if completed is not None:
            print("stdout:", file=sys.stderr)
            print(completed.stdout, file=sys.stderr)
            print("stderr:", file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
        return 1

    print("integration tests: PASS (2 tests)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
