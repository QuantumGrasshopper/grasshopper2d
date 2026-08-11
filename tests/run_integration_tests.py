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


def require_invalid_invocation(executable, arguments, expected_error, case_name):
    sentinel = "existing result must remain unchanged\n"
    with tempfile.TemporaryDirectory(prefix="grasshopper2d-invalid-cli-") as directory:
        working_directory = pathlib.Path(directory)
        result_path = working_directory / "result.dat"
        result_path.write_text(sentinel, encoding="utf-8")
        completed = subprocess.run(
            [str(executable), *arguments],
            cwd=working_directory,
            capture_output=True,
            text=True,
            timeout=30,
            check=False,
        )

        require(completed.returncode != 0, f"{case_name} was not rejected")
        require(expected_error in completed.stderr,
                f"{case_name} did not report {expected_error!r}; "
                f"stderr was {completed.stderr!r}")
        require(result_path.read_text(encoding="utf-8") == sentinel,
                f"{case_name} truncated result.dat")


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


# Independent reference implementation of the delta-function discretizations,
# to compute direct pairwise probabilities without using production neighbor-table code
def reference_contribution_energy(normalized_offset, delta_option):
    if normalized_offset > 2.0:
        return 0.0

    if delta_option == 0:
        return (1.0 + math.cos(math.pi * normalized_offset / 2.0)) / 4.0

    if normalized_offset < 1.0:
        return (17.0 / 48.0 + math.sqrt(3.0) * math.pi / 108.0
                + normalized_offset / 4.0 - normalized_offset ** 2 / 4.0
                + (1.0 - 2.0 * normalized_offset)
                * math.sqrt(1.0 + 12.0 * normalized_offset
                            * (1.0 - normalized_offset)) / 16.0
                - math.sqrt(3.0)
                * math.asin(math.sqrt(3.0) * (2.0 * normalized_offset - 1.0) / 2.0)
                / 12.0)
    if normalized_offset < 2.0:
        return (55.0 / 48.0 - math.sqrt(3.0) * math.pi / 108.0
                - 13.0 * normalized_offset / 12.0 + normalized_offset ** 2 / 4.0
                + (2.0 * normalized_offset - 3.0)
                * math.sqrt(36.0 * normalized_offset - 23.0
                            - 12.0 * normalized_offset ** 2) / 48.0
                + math.sqrt(3.0)
                * math.asin(math.sqrt(3.0) * (2.0 * normalized_offset - 3.0) / 2.0)
                / 36.0)
    return 0.0


def direct_pairwise_probability(coordinates, grid_size, hopping_distance, delta_option):
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
            energy += reference_contribution_energy(normalized_offset, delta_option)

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

        hopping_distance = 1.5
        valid_reach_cases = [
            ("odd minimum-valid grid", 9, 0, [38, 42]),
            ("even minimum-valid grid", 10, 1, [53, 57]),
        ]

        for case_name, case_grid_size, delta_option, coordinates in valid_reach_cases:
            command = [
                str(executable),
                "-N", "2",
                "-gridsize", str(case_grid_size),
                "-d", str(hopping_distance),
                "-hours", "1",
                "-steps", "1",
                "-tempsteps", "10",
                "-inittemp", "20",
                "-fintemp", "0.05",
                "-annealsteps", "100",
                "-configoutput", "0",
                "-initconf", "load",
                "-delta", str(delta_option),
                "-NNint", "0",
                "-randomseed", "12345",
            ]

            with tempfile.TemporaryDirectory(prefix="grasshopper2d-valid-reach-") as directory:
                working_directory = pathlib.Path(directory)
                configuration = "".join(f"{coordinate}\n" for coordinate in coordinates)
                (working_directory / "initconf.dat").write_text(
                    configuration, encoding="utf-8")
                completed = subprocess.run(
                    command,
                    cwd=working_directory,
                    capture_output=True,
                    text=True,
                    timeout=30,
                    check=False,
                )

                require(completed.returncode == 0,
                        f"{case_name} exited with status {completed.returncode}")

                energy_lines = (working_directory / "energies.dat").read_text(
                    encoding="utf-8").splitlines()
                require(energy_lines, f"{case_name} produced an empty energies.dat")
                try:
                    table_probability = float(energy_lines[0])
                except ValueError as error:
                    raise IntegrationTestFailure(
                        f"{case_name} energies.dat does not begin with a number") from error

                direct_probability = direct_pairwise_probability(
                    coordinates, case_grid_size, hopping_distance, delta_option)
                require(math.isclose(table_probability, direct_probability,
                                     rel_tol=0.0, abs_tol=5.1e-7),
                        f"{case_name} neighbor-table probability "
                        f"{table_probability:.17g} does not match direct pairwise probability "
                        f"{direct_probability:.17g}")

        invalid_reach_cases = [
            ("undersized odd grid", 7, 0),
            ("undersized even grid", 8, 1),
        ]
        expected_error = "Grid size is too small for the hopping-distance support."

        for case_name, case_grid_size, delta_option in invalid_reach_cases:
            arguments = [
                "-N", "2",
                "-gridsize", str(case_grid_size),
                "-d", str(hopping_distance),
                "-delta", str(delta_option),
            ]
            require_invalid_invocation(executable, arguments, expected_error, case_name)

        with tempfile.TemporaryDirectory(prefix="grasshopper2d-defaults-") as directory:
            working_directory = pathlib.Path(directory)
            command = [str(executable), "-d", "0.25", "-N", "2", "-gridsize", "5"]
            completed = subprocess.run(
                command,
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )
            require(completed.returncode == 0,
                    f"default-option run exited with status {completed.returncode}")
            result_text = (working_directory / "result.dat").read_text(encoding="utf-8")
            require("Initial temperature: 20" in result_text,
                    "default-option run did not use the default initial temperature")
            require("Final temperature: 0.01" in result_text,
                    "default-option run did not use the default final temperature")
            require("Number of annealing steps: 1000" in result_text,
                    "default-option run did not use the default annealing steps")

        invalid_cli_cases = [
            ("missing required d", [], "Required option -d"),
            ("unknown option", ["-d", "0.25", "-bogus", "1"],
             "Unknown option: -bogus"),
            ("duplicate option", ["-d", "0.25", "-d", "0.5"],
             "Duplicate option: -d"),
            ("missing option value", ["-d"], "Missing value for option -d"),
            ("partial numeric value", ["-d", "0.25", "-steps", "10abc"],
             "Invalid value for -steps"),
            ("non-finite floating-point value", ["-d", "0.25", "-NNint", "nan"],
             "Invalid value for -NNint"),
            ("negative unsigned value", ["-d", "0.25", "-randomseed", "-1"],
             "Invalid value for -randomseed"),
            ("out-of-range value", ["-d", "0.25", "-N", "4294967296"],
             "outside the destination type range"),
            ("invalid initialization mode", ["-d", "0.25", "-initconf", "Random"],
             "-initconf must be exactly random, disk, or load"),
            ("invalid delta option", ["-d", "0.25", "-delta", "2"],
             "-delta must be exactly 0 or 1"),
            ("full occupancy", ["-d", "0.1", "-N", "4", "-gridsize", "2"],
             "N must be smaller than the grid area"),
        ]

        for case_name, arguments, expected_cli_error in invalid_cli_cases:
            require_invalid_invocation(
                executable, arguments, expected_cli_error, case_name)

        output_policy_arguments = [
            "-N", "2",
            "-gridsize", "5",
            "-d", "0.25",
            "-hours", "0",
            "-annealsteps", "100",
            "-configoutput", "0",
            "-randomseed", "12345",
        ]

        with tempfile.TemporaryDirectory(prefix="grasshopper2d-no-overwrite-") as directory:
            working_directory = pathlib.Path(directory)
            stale_config = working_directory / "config.dat"
            sentinel = "configuration from an earlier run\n"
            stale_config.write_text(sentinel, encoding="utf-8")
            completed = subprocess.run(
                [str(executable), *output_policy_arguments],
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode != 0,
                    "default overwrite policy accepted an existing output")
            require("Output artifact already exists: config.dat" in completed.stderr,
                    "default overwrite policy did not identify the existing output")
            require(stale_config.read_text(encoding="utf-8") == sentinel,
                    "default overwrite policy changed the existing output")
            require(not (working_directory / "result.dat").exists(),
                    "default overwrite rejection created result.dat")

        with tempfile.TemporaryDirectory(prefix="grasshopper2d-overwrite-") as directory:
            working_directory = pathlib.Path(directory)
            stale_result = working_directory / "result.dat"
            stale_config = working_directory / "config.dat"
            stale_result.write_text("result from an earlier run\n", encoding="utf-8")
            stale_config.write_text("stale configuration output\n", encoding="utf-8")
            completed = subprocess.run(
                [str(executable), *output_policy_arguments, "-overwrite", "1"],
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode == 0,
                    f"overwrite run exited with status {completed.returncode}")
            require("2D Grasshopper with Simulated Annealing" in
                    stale_result.read_text(encoding="utf-8"),
                    "overwrite run did not replace result.dat")
            require(not stale_config.exists(),
                    "overwrite run left stale config.dat when configoutput was zero")

        with tempfile.TemporaryDirectory(prefix="grasshopper2d-load-overwrite-") as directory:
            working_directory = pathlib.Path(directory)
            initial_configuration = working_directory / "initconf.dat"
            initial_contents = "6\n18\n"
            initial_configuration.write_text(initial_contents, encoding="utf-8")
            (working_directory / "config.dat").write_text(
                "stale configuration output\n", encoding="utf-8")
            completed = subprocess.run(
                [str(executable), *output_policy_arguments,
                 "-initconf", "load", "-overwrite", "1"],
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode == 0,
                    f"load overwrite run exited with status {completed.returncode}")
            require(initial_configuration.read_text(encoding="utf-8") == initial_contents,
                    "load overwrite run changed initconf.dat")
            require(not (working_directory / "config.dat").exists(),
                    "load overwrite run left stale config.dat")

        with tempfile.TemporaryDirectory(prefix="grasshopper2d-cleanup-failure-") as directory:
            working_directory = pathlib.Path(directory)

            stale_result = working_directory / "result.dat"
            result_sentinel = "result from an earlier run\n"
            stale_result.write_text(result_sentinel, encoding="utf-8")

            blocked_artifact = working_directory / "config.dat"
            blocked_artifact.mkdir()
            completed = subprocess.run(
                [str(executable), *output_policy_arguments, "-overwrite", "1"],
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode != 0, "output cleanup failure was ignored")
            require("Output artifact is a directory: config.dat" in completed.stderr,
                    "directory output artifact was not identified")
            require(stale_result.read_text(encoding="utf-8") == result_sentinel,
                    "cleanup failure changed an earlier output before preflight completed")
            require(blocked_artifact.is_dir(),
                    "directory output artifact was removed")

    except (IntegrationTestFailure, OSError, subprocess.SubprocessError) as error:
        print(f"integration tests: FAIL: {error}", file=sys.stderr)
        print("command: " + " ".join(command), file=sys.stderr)
        if completed is not None:
            print("stdout:", file=sys.stderr)
            print(completed.stdout, file=sys.stderr)
            print("stderr:", file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
        return 1

    print("integration tests: PASS (21 tests)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
