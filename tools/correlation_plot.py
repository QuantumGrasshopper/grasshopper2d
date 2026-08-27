#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Olga Goulko

import argparse
import pathlib
import sys

import matplotlib.pyplot as plt
import numpy as np


EXPECTED_HEADER = (
    "# cell x y occupation local_grasshopper_probability nn_fraction"
)

FIELDS = {
    "occupation": (3, "Occupation", False),
    "local-grasshopper-probability": (
        4, "Normalized local grasshopper probability", False),
    "occupied-local-grasshopper-probability": (
        4, "Normalized local grasshopper probability (occupied cells)", True),
}


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Plot spatial fields calculated by the C++ correlation tool.")
    parser.add_argument(
        "data", nargs="?", default="correlations.dat",
        help="correlation data file (default: correlations.dat)")
    parser.add_argument(
        "--field", action="append", choices=FIELDS,
        help="field to plot; repeat to select multiple fields (default: all)")
    parser.add_argument(
        "--save", metavar="FILE",
        help="save the figure instead of displaying it interactively")
    return parser.parse_args()


def read_spatial_data(filename):
    path = pathlib.Path(filename)
    with path.open(encoding="utf-8") as input_file:
        header = input_file.readline().rstrip("\n")
    if header != EXPECTED_HEADER:
        raise ValueError(
            f"{path} does not have the expected correlation-data schema")

    data = np.loadtxt(path, comments="#")
    data = np.atleast_2d(data)
    if data.shape[1] != 6 or data.shape[0] == 0:
        raise ValueError(f"{path} must contain six columns and at least one row")
    if not np.all(np.isfinite(data)):
        raise ValueError(f"{path} contains non-finite values")

    integer_columns = data[:, [0, 1, 2, 3]]
    if not np.all(integer_columns == np.floor(integer_columns)):
        raise ValueError(f"{path} contains non-integer index or occupation data")

    cells = data[:, 0].astype(int)
    x_coordinates = data[:, 1].astype(int)
    y_coordinates = data[:, 2].astype(int)
    occupation = data[:, 3].astype(int)
    width = int(x_coordinates.max()) + 1
    height = int(y_coordinates.max()) + 1

    if width <= 0 or height <= 0 or width != height:
        raise ValueError(f"{path} does not describe a nonempty square grid")
    expected_cells = np.arange(width * height)
    if data.shape[0] != expected_cells.size or not np.array_equal(cells, expected_cells):
        raise ValueError(f"{path} is not complete flattened row-major grid data")
    if not np.array_equal(x_coordinates, expected_cells % width):
        raise ValueError(f"{path} has inconsistent x coordinates")
    if not np.array_equal(y_coordinates, expected_cells // width):
        raise ValueError(f"{path} has inconsistent y coordinates")
    if not np.all((occupation == 0) | (occupation == 1)):
        raise ValueError(f"{path} contains occupation values other than 0 or 1")

    return data, height, width


def main():
    arguments = parse_arguments()
    selected_fields = arguments.field or [
        "occupation",
        "local-grasshopper-probability",
        "occupied-local-grasshopper-probability",
    ]

    try:
        data, height, width = read_spatial_data(arguments.data)
    except (OSError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1

    figure, axes = plt.subplots(
        1, len(selected_fields),
        figsize=(5 * len(selected_fields), 4),
        squeeze=False)

    for axis, field_name in zip(axes[0], selected_fields):
        column, title, mask_unoccupied = FIELDS[field_name]
        field = data[:, column].reshape(height, width)
        if mask_unoccupied:
            occupation = data[:, 3].reshape(height, width)
            field = np.ma.masked_where(occupation != 1, field)
        image = axis.imshow(
            field,
            interpolation="none", origin="lower", cmap="Greens")
        axis.set_title(title)
        axis.set_xlabel("x")
        axis.set_ylabel("y")
        figure.colorbar(image, ax=axis)

    figure.tight_layout()
    if arguments.save:
        figure.savefig(arguments.save, dpi=150)
    else:
        plt.show()
    return 0


if __name__ == "__main__":
    sys.exit(main())
