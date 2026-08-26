#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Olga Goulko

import contextlib
import math
import pathlib
import subprocess
import sys
import tempfile


class IntegrationTestFailure(RuntimeError):
    pass


class IntegrationTestSuite:
    def __init__(self):
        self.test_count = 0
        self.failures = []

    @contextlib.contextmanager
    def case(self, name):
        self.test_count += 1
        try:
            yield
        except Exception as error:
            self.failures.append((name, error))

    def finish(self):
        if self.failures:
            print(f"integration tests: FAIL "
                  f"({len(self.failures)} of {self.test_count} tests failed)",
                  file=sys.stderr)
            for name, error in self.failures:
                print(f"- {name}: {error}", file=sys.stderr)
            return 1

        print(f"integration tests: PASS ({self.test_count} tests)")
        return 0


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
    return coordinates


def read_configuration_snapshots(path, expected_count):
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise IntegrationTestFailure(f"could not read {path.name}: {error}") from error

    snapshots = []
    for line_number, line in enumerate(lines, start=1):
        fields = line.split()
        require(len(fields) == expected_count + 1,
                f"{path.name} line {line_number} has {len(fields)} fields, "
                f"expected {expected_count + 1}")
        try:
            coordinates = [int(field) for field in fields[:-1]]
            energy = float(fields[-1])
        except ValueError as error:
            raise IntegrationTestFailure(
                f"could not parse {path.name} line {line_number}: {error}") from error
        snapshots.append((coordinates, energy))
    return snapshots


def read_result_values(path):
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise IntegrationTestFailure(f"could not read {path.name}: {error}") from error

    values = {}
    for line in lines:
        label, separator, value = line.partition(": ")
        if separator:
            values[label] = value
    return values


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


def direct_pairwise_energy(coordinates, grid_size, hopping_distance, delta_option):
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

    return energy


def direct_pairwise_probability(coordinates, grid_size, hopping_distance, delta_option):
    energy = direct_pairwise_energy(
        coordinates, grid_size, hopping_distance, delta_option)
    normalization = math.pi * hopping_distance * len(coordinates) ** 1.5
    return energy / normalization


def direct_nearest_neighbor_bonds(coordinates, grid_size):
    occupied = set(coordinates)
    bonds = 0
    for coordinate in coordinates:
        if coordinate % grid_size != grid_size - 1 and coordinate + 1 in occupied:
            bonds += 1
        if coordinate + grid_size in occupied:
            bonds += 1
    return bonds


def require_close(actual, expected, description):
    require(math.isclose(actual, expected, rel_tol=0.0, abs_tol=1.0e-12),
            f"{description}: {actual:.17g} != {expected:.17g}")


def main():
    if len(sys.argv) != 3:
        print(f"usage: {pathlib.Path(sys.argv[0]).name} "
              "GRASSHOPPER_EXECUTABLE CORRELATION_EXECUTABLE",
              file=sys.stderr)
        return 2

    executable = pathlib.Path(sys.argv[1]).resolve()
    correlation_executable = pathlib.Path(sys.argv[2]).resolve()
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
    suite = IntegrationTestSuite()

    try:
        require(executable.is_file(), f"executable not found: {executable}")
        require(correlation_executable.is_file(),
                f"executable not found: {correlation_executable}")

        with suite.case("basic simulation outputs"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-integration-") as directory:
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

        with suite.case("final and best objectives match direct recomputation"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-objective-audit-") as directory:
            working_directory = pathlib.Path(directory)
            initial_coordinates = [44, 45, 54, 55]
            total_spins = len(initial_coordinates)
            grid_size = 10
            hopping_distance = 1.5
            delta_option = 0
            nearest_neighbor_interaction = 0.75
            (working_directory / "initconf.dat").write_text(
                "".join(f"{coordinate}\n" for coordinate in initial_coordinates),
                encoding="utf-8")
            command = [
                str(executable),
                "-N", str(total_spins),
                "-gridsize", str(grid_size),
                "-d", str(hopping_distance),
                "-hours", "1",
                "-steps", "12",
                "-tempsteps", "10",
                "-inittemp", "100",
                "-fintemp", "1",
                "-annealsteps", "100",
                "-configoutput", "0",
                "-initconf", "load",
                "-delta", str(delta_option),
                "-NNint", str(nearest_neighbor_interaction),
                "-randomseed", "12345",
            ]
            completed = subprocess.run(
                command,
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )
            require(completed.returncode == 0,
                    f"objective-audit run exited with status {completed.returncode}")

            final_coordinates = read_configuration(
                working_directory / "finconf.dat", total_spins, grid_size ** 2)
            best_coordinates = read_configuration(
                working_directory / "bestconf.dat", total_spins, grid_size ** 2)
            require(set(final_coordinates) != set(initial_coordinates),
                    "objective-audit run did not exercise an accepted move")

            final_grasshopper_energy = direct_pairwise_energy(
                final_coordinates, grid_size, hopping_distance, delta_option)
            final_nearest_neighbor_bonds = direct_nearest_neighbor_bonds(
                final_coordinates, grid_size)
            final_objective = (final_grasshopper_energy
                               + nearest_neighbor_interaction
                               * final_nearest_neighbor_bonds)
            best_grasshopper_energy = direct_pairwise_energy(
                best_coordinates, grid_size, hopping_distance, delta_option)
            best_nearest_neighbor_bonds = direct_nearest_neighbor_bonds(
                best_coordinates, grid_size)
            best_objective = (best_grasshopper_energy
                              + nearest_neighbor_interaction
                              * best_nearest_neighbor_bonds)
            normalization = (math.pi * hopping_distance
                             * total_spins ** 1.5)
            result_values = read_result_values(working_directory / "result.dat")

            require_close(float(result_values["final grasshopper energy"]),
                          final_grasshopper_energy,
                          "final grasshopper energy")
            require(int(result_values["final nearest neighbor bonds"])
                    == final_nearest_neighbor_bonds,
                    "final nearest-neighbor bond count does not match direct recount")
            require_close(float(result_values["final total MC objective"]),
                          final_objective, "final total MC objective")
            require_close(float(result_values["best total MC objective"]),
                          best_objective, "best total MC objective")
            require_close(float(result_values["final grasshopper probability"]),
                          final_grasshopper_energy / normalization,
                          "final grasshopper probability")
            require_close(float(result_values["final nearest neighbor probability"]),
                          final_nearest_neighbor_bonds / (2.0 * total_spins),
                          "final nearest-neighbor probability")
            require_close(float(result_values["final normalized MC objective"]),
                          final_objective / normalization,
                          "final normalized MC objective")
            require_close(float(result_values["best normalized MC objective"]),
                          best_objective / normalization,
                          "best normalized MC objective")

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

            with suite.case(case_name), \
                    tempfile.TemporaryDirectory(prefix="grasshopper2d-valid-reach-") as directory:
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
                                     rel_tol=0.0, abs_tol=1.0e-12),
                        f"{case_name} neighbor-table probability "
                        f"{table_probability:.17g} does not match direct pairwise probability "
                        f"{direct_probability:.17g}")

        invalid_reach_cases = [
            ("undersized odd grid", 7, 0),
            ("undersized even grid", 8, 1),
        ]
        expected_error = "Grid size is too small for the requested interaction-distance support."

        for case_name, case_grid_size, delta_option in invalid_reach_cases:
            arguments = [
                "-N", "2",
                "-gridsize", str(case_grid_size),
                "-d", str(hopping_distance),
                "-delta", str(delta_option),
            ]
            with suite.case(case_name):
                require_invalid_invocation(executable, arguments, expected_error, case_name)

        with suite.case("default simulation options"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-defaults-") as directory:
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

        with suite.case("automatic grid sizing"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-automatic-grid-") as directory:
            working_directory = pathlib.Path(directory)
            total_spins = 4
            hopping_distance = 0.25
            command = [
                str(executable),
                "-d", str(hopping_distance),
                "-N", str(total_spins),
                "-hours", "0",
                "-randomseed", "12345",
            ]
            completed = subprocess.run(
                command,
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )
            require(completed.returncode == 0,
                    f"automatic-grid run exited with status {completed.returncode}")
            result_values = read_result_values(working_directory / "result.dat")
            reported_grid_size = int(result_values["Size of grid"])
            cell_size = 1.0 / math.sqrt(total_spins)
            required_reach = math.ceil(hopping_distance / cell_size) + 1
            require((reported_grid_size - 1) // 2 >= required_reach,
                    "automatically selected grid does not support the hopping distance")
            require(total_spins < reported_grid_size ** 2,
                    "automatically selected grid is fully occupied")
            for filename in ("initconf.dat", "finconf.dat", "bestconf.dat"):
                read_configuration(working_directory / filename,
                                   total_spins, reported_grid_size ** 2)

        invalid_cli_cases = [
            ("partial numeric value", ["-d", "0.25", "-steps", "10abc"],
             "Invalid value for -steps"),
            ("full occupancy", ["-d", "0.1", "-N", "4", "-gridsize", "2"],
             "N must be smaller than the grid area"),
        ]

        for case_name, arguments, expected_cli_error in invalid_cli_cases:
            with suite.case(case_name):
                require_invalid_invocation(
                    executable, arguments, expected_cli_error, case_name)

        with suite.case("single-step annealing schedule"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-small-anneal-") as directory:
            working_directory = pathlib.Path(directory)
            command = [
                str(executable),
                "-N", "2",
                "-gridsize", "5",
                "-d", "0.25",
                "-hours", "1",
                "-steps", "2",
                "-tempsteps", "2",
                "-annealsteps", "1",
                "-configoutput", "0",
                "-randomseed", "12345",
            ]
            completed = subprocess.run(
                command,
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode == 0,
                    f"annealsteps=1 run exited with status {completed.returncode}")
            result_text = (working_directory / "result.dat").read_text(encoding="utf-8")
            require("Finished after 2 steps" in result_text,
                    "annealsteps=1 run did not complete both steps")
            temperature_lines = (working_directory / "temperatures.dat").read_text(
                encoding="utf-8").splitlines()
            require(temperature_lines and temperature_lines[0].startswith("2\t"),
                    "annealsteps=1 run did not execute the cooling step")

        with suite.case("single final configuration snapshot"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-config-output-") as directory:
            working_directory = pathlib.Path(directory)
            command = [
                str(executable),
                "-N", "2",
                "-gridsize", "9",
                "-d", "1.5",
                "-hours", "1",
                "-steps", "2",
                "-tempsteps", "2",
                "-annealsteps", "100",
                "-configoutput", "1",
                "-randomseed", "12345",
            ]
            completed = subprocess.run(
                command,
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode == 0,
                    f"config-output run exited with status {completed.returncode}")
            config_path = working_directory / "config.dat"
            require(config_path.is_file(), "config-output run did not create config.dat")
            snapshots = read_configuration_snapshots(config_path, expected_count=2)
            require(len(snapshots) == 1,
                    f"configoutput=1 produced {len(snapshots)} rows, expected 1")
            final_coordinates = read_configuration(
                working_directory / "finconf.dat", expected_count=2, grid_area=81)
            snapshot_coordinates, snapshot_energy = snapshots[0]
            require(snapshot_coordinates == final_coordinates,
                    "configoutput=1 row does not match finconf.dat")
            require_close(
                snapshot_energy,
                direct_pairwise_probability(snapshot_coordinates, 9, 1.5, 0),
                "configoutput=1 energy")

        with suite.case("scheduled configuration snapshots"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-config-schedule-") as directory:
            working_directory = pathlib.Path(directory)
            command = [
                str(executable),
                "-N", "2",
                "-gridsize", "9",
                "-d", "1.5",
                "-hours", "1",
                "-steps", "14",
                "-tempsteps", "2",
                "-inittemp", "4",
                "-fintemp", "1",
                "-annealsteps", "2",
                "-configoutput", "10",
                "-randomseed", "12345",
            ]
            completed = subprocess.run(
                command,
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode == 0,
                    f"configuration-schedule run exited with status {completed.returncode}")
            snapshots = read_configuration_snapshots(
                working_directory / "config.dat", expected_count=2)
            require(len(snapshots) == 3,
                    f"configuration-schedule run produced {len(snapshots)} rows, expected 3")
            initial_coordinates = read_configuration(
                working_directory / "initconf.dat", expected_count=2, grid_area=81)
            final_coordinates = read_configuration(
                working_directory / "finconf.dat", expected_count=2, grid_area=81)
            require(snapshots[0][0] == initial_coordinates,
                    "configuration-schedule first row does not match initconf.dat")
            require(snapshots[-1][0] == final_coordinates,
                    "configuration-schedule last row does not match finconf.dat")
            for snapshot_index, (coordinates, energy) in enumerate(snapshots):
                require_close(
                    energy,
                    direct_pairwise_probability(coordinates, 9, 1.5, 0),
                    f"configuration-schedule snapshot {snapshot_index} energy")

        output_policy_arguments = [
            "-N", "2",
            "-gridsize", "5",
            "-d", "0.25",
            "-hours", "0",
            "-annealsteps", "100",
            "-configoutput", "0",
            "-randomseed", "12345",
        ]

        with suite.case("default no-overwrite policy"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-no-overwrite-") as directory:
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

        with suite.case("explicit overwrite policy"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-overwrite-") as directory:
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

        with suite.case("load initialization preserves its input"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-load-overwrite-") as directory:
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

        with suite.case("output cleanup preflight failure"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-cleanup-failure-") as directory:
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

        with suite.case("correlation output"), \
                tempfile.TemporaryDirectory(prefix="grasshopper2d-correlation-") as directory:
            working_directory = pathlib.Path(directory)
            coordinates = [40, 41]
            correlation_distance = 1.5
            correlation_grid_size = 9
            delta_option = 0
            metadata_path = working_directory / "result.dat"
            configuration_path = working_directory / "configuration.dat"
            output_path = working_directory / "correlations.dat"

            metadata_path.write_text(
                "Hopping distance: 1.5\n"
                "Size of grid: 9\n"
                "Option for delta-function discretization: 0\n"
                "Total number of spins: 2\n",
                encoding="utf-8")
            configuration_path.write_text("40\n41\n", encoding="utf-8")
            output_path.write_text("stale correlation output\n", encoding="utf-8")
            command = [
                str(correlation_executable),
                "-config", str(configuration_path),
            ]
            completed = subprocess.run(
                command,
                cwd=working_directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            require(completed.returncode == 0,
                    f"correlation tool exited with status {completed.returncode}")
            require(completed.stderr == "", f"unexpected warning: {completed.stderr!r}")

            output_lines = output_path.read_text(encoding="utf-8").splitlines()
            expected_header = (
                "# cell x y occupation local_grasshopper_probability nn_fraction")
            require(output_lines and output_lines[0] == expected_header,
                    "correlation output header does not match its documented schema")
            require(len(output_lines) == correlation_grid_size ** 2 + 1,
                    "correlation output does not contain one row per grid site")

            rows = []
            for cell, line in enumerate(output_lines[1:]):
                fields = line.split()
                require(len(fields) == 6,
                        f"correlation row {cell} does not contain six columns")
                try:
                    row = [float(field) for field in fields]
                except ValueError as error:
                    raise IntegrationTestFailure(
                        f"correlation row {cell} contains invalid numeric data") from error
                require(row[0] == cell,
                        f"correlation row {cell} has the wrong flattened index")
                require(row[1] == cell % correlation_grid_size
                        and row[2] == cell // correlation_grid_size,
                        f"correlation row {cell} has incorrect lattice coordinates")
                rows.append(row)

            require(rows[40][3] == 1.0 and rows[41][3] == 1.0,
                    "occupied correlation rows were not marked occupied")
            require(rows[31][3] == 0.0,
                    "unoccupied correlation row was not emitted")

            cell_size = 1.0 / math.sqrt(len(coordinates))
            normalized_offset = abs(correlation_distance - cell_size) / cell_size
            expected_local_probability = (
                reference_contribution_energy(normalized_offset, delta_option)
                / (2.0 * math.pi * correlation_distance * math.sqrt(len(coordinates))))
            require(math.isclose(rows[40][4], expected_local_probability,
                                 rel_tol=0.0, abs_tol=1.0e-12),
                    "normalized local grasshopper probability is incorrect")
            require(rows[40][5] == 0.25,
                    "occupied-site nearest-neighbor fraction is incorrect")
            require(rows[31][5] == 0.25,
                    "unoccupied-site nearest-neighbor fraction is incorrect")

            stdout_values = {}
            for line in completed.stdout.splitlines():
                label, separator, value = line.partition(": ")
                if separator:
                    stdout_values[label] = value
            required_stdout_labels = {
                "Requested distance",
                "Global grasshopper probability",
                "Global nearest-neighbor probability",
                "Output file",
            }
            require(required_stdout_labels <= stdout_values.keys(),
                    "correlation stdout is missing a required summary field")
            expected_global_probability = direct_pairwise_probability(
                coordinates, correlation_grid_size,
                correlation_distance, delta_option)
            require(math.isclose(float(stdout_values["Requested distance"]),
                                 correlation_distance,
                                 rel_tol=0.0, abs_tol=1.0e-15),
                    "requested distance was not reported correctly")
            require(math.isclose(float(stdout_values["Global grasshopper probability"]),
                                 expected_global_probability,
                                 rel_tol=0.0, abs_tol=1.0e-12),
                    "global grasshopper probability is incorrect")
            require(float(stdout_values["Global nearest-neighbor probability"]) == 0.25,
                    "global nearest-neighbor probability is incorrect")
            require(stdout_values["Output file"] == "correlations.dat",
                    "correlation output filename was not reported")

    except Exception as error:
        suite.failures.append(("integration test harness", error))

    return suite.finish()


if __name__ == "__main__":
    sys.exit(main())
