#!/usr/bin/env bash

set -u
set -o pipefail

usage() {
    printf '%s\n' \
        "Usage: $0 --steps STEPS [--repetitions REPS] [--cpu CPU]" \
        "          [--seed SEED] [--output-dir DIR]" \
        "" \
        "The production benchmark step count has no default and must be supplied."
}

steps=""
repetitions=5
cpu=0
seed=12345
output_dir=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --steps)
            [[ $# -ge 2 ]] || { usage >&2; exit 2; }
            steps=$2
            shift 2
            ;;
        --repetitions)
            [[ $# -ge 2 ]] || { usage >&2; exit 2; }
            repetitions=$2
            shift 2
            ;;
        --cpu)
            [[ $# -ge 2 ]] || { usage >&2; exit 2; }
            cpu=$2
            shift 2
            ;;
        --seed)
            [[ $# -ge 2 ]] || { usage >&2; exit 2; }
            seed=$2
            shift 2
            ;;
        --output-dir)
            [[ $# -ge 2 ]] || { usage >&2; exit 2; }
            output_dir=$2
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            printf 'Unknown option: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

[[ $steps =~ ^[0-9]+$ && $steps -gt 0 ]] || {
    printf '%s\n' "--steps must be supplied as a positive integer" >&2
    exit 2
}
[[ $repetitions =~ ^[0-9]+$ && $repetitions -gt 0 ]] || {
    printf '%s\n' "--repetitions must be a positive integer" >&2
    exit 2
}
[[ $cpu =~ ^[0-9]+$ ]] || {
    printf '%s\n' "--cpu must identify one non-negative CPU" >&2
    exit 2
}
[[ $seed =~ ^[0-9]+$ ]] || {
    printf '%s\n' "--seed must be a non-negative integer" >&2
    exit 2
}

export LC_ALL=C

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "$script_dir/.." && pwd)
time_bin=/usr/bin/time

command -v taskset >/dev/null 2>&1 || {
    printf '%s\n' "taskset is required" >&2
    exit 1
}
[[ -x $time_bin ]] || {
    printf '%s\n' "$time_bin is required" >&2
    exit 1
}

variants=(vla unique vector)
declare -A executables=(
    [vla]="$script_dir/grasshopper_storage_vla"
    [unique]="$script_dir/grasshopper_storage_unique"
    [vector]="$script_dir/grasshopper_storage_vector"
)

for variant in "${variants[@]}"; do
    [[ -x ${executables[$variant]} ]] || {
        printf 'Missing executable: %s\n' "${executables[$variant]}" >&2
        printf '%s\n' "Run: make -C benchmarks storage-benchmarks" >&2
        exit 1
    }
done

if [[ -z $output_dir ]]; then
    timestamp=$(date -u +%Y%m%dT%H%M%SZ)
    output_dir="$script_dir/results/storage-$timestamp"
fi

mkdir -p "$output_dir/raw"
metadata_file="$output_dir/metadata.txt"
manifest_file="$output_dir/manifest.tsv"

compiler=${CXX:-g++}
{
    printf 'timestamp_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf 'repository=%s\n' "$repo_root"
    printf 'git_commit=%s\n' "$(git -C "$repo_root" rev-parse HEAD 2>/dev/null || printf unknown)"
    printf 'git_status_begin\n'
    git -C "$repo_root" status --short 2>/dev/null || true
    printf 'git_status_end\n'
    printf 'build_command=make -C benchmarks storage-benchmarks\n'
    printf 'compiler_command=%s\n' "$compiler"
    "$compiler" --version 2>/dev/null | head -n 1 || true
    make -C "$script_dir" -pn storage-benchmarks 2>/dev/null \
        | awk -F ' = ' '/^(CXX|CXXFLAGS|LDFLAGS) = / { print "make_" $1 "=" $2 }' \
        || true
    printf 'uname=%s\n' "$(uname -a)"
    printf 'stack_limit_kib=%s\n' "$(ulimit -s)"
    printf 'cpu=%s\n' "$cpu"
    printf 'seed=%s\n' "$seed"
    printf 'steps=%s\n' "$steps"
    printf 'repetitions=%s\n' "$repetitions"
    if [[ -r /sys/devices/system/cpu/cpu${cpu}/cpufreq/scaling_governor ]]; then
        printf 'cpu_governor=%s\n' "$(<"/sys/devices/system/cpu/cpu${cpu}/cpufreq/scaling_governor")"
    fi
    printf 'lscpu_begin\n'
    lscpu 2>/dev/null || true
    printf 'lscpu_end\n'
} >"$metadata_file"

printf 'case\tvariant\trepetition\tN\tgrid_size\td\tsteps\tseed\texit_status\tstdout\tstderr\ttime\tcommand\n' \
    >"$manifest_file"

cases=(
    "n10000_g200_d030:10000:200:0.3"
    "n20000_g200_d030:20000:200:0.3"
    "n10000_g380_d030:10000:380:0.3"
)

failures=0

# Rotate variant order between repetitions to reduce fixed ordering bias.
for ((repetition=1; repetition<=repetitions; ++repetition)); do
    rotation=$(( (repetition - 1) % ${#variants[@]} ))

    for case_spec in "${cases[@]}"; do
        IFS=: read -r case_name n grid_size d <<<"$case_spec"

        for ((offset=0; offset<${#variants[@]}; ++offset)); do
            variant_index=$(( (rotation + offset) % ${#variants[@]} ))
            variant=${variants[$variant_index]}
            executable=${executables[$variant]}

            run_dir="$output_dir/raw/$case_name/$variant"
            mkdir -p "$run_dir"
            run_id=$(printf 'run-%03d' "$repetition")
            stdout_file="$run_dir/$run_id.stdout"
            stderr_file="$run_dir/$run_id.stderr"
            time_file="$run_dir/$run_id.time"
            status_file="$run_dir/$run_id.status"
            command_file="$run_dir/$run_id.command"

            command=(
                taskset -c "$cpu" "$executable"
                --N "$n"
                --grid-size "$grid_size"
                --d "$d"
                --steps "$steps"
                --seed "$seed"
            )

            printf '%q ' "${command[@]}" >"$command_file"
            printf '\n' >>"$command_file"

            "$time_bin" -v -o "$time_file" "${command[@]}" \
                >"$stdout_file" 2>"$stderr_file"
            status=$?
            printf '%s\n' "$status" >"$status_file"

            if [[ $status -ne 0 ]]; then
                failures=$((failures + 1))
            fi

            printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
                "$case_name" "$variant" "$repetition" "$n" "$grid_size" "$d" \
                "$steps" "$seed" "$status" "$stdout_file" "$stderr_file" \
                "$time_file" "$command_file" >>"$manifest_file"
        done
    done
done

printf 'Raw benchmark results: %s\n' "$output_dir"
if [[ $failures -ne 0 ]]; then
    printf 'Failed runs: %s\n' "$failures" >&2
    exit 1
fi
