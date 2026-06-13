#!/usr/bin/env bash
set -E -e -u -o pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
readonly SCRIPT_DIR
readonly WORKER_SRC="${SCRIPT_DIR}/cgroup-sched-worker.c"
readonly WORKER_BIN="${SCRIPT_DIR}/cgroup-sched-worker.out"

CGROUP_ROOT=""
declare -a CREATED_CGROUPS=()
declare -a WORKER_PIDS=()
readonly SCOPE_ENV=MULTI_CPU_DEMO_SCOPE

function die() {
    printf 'error: %s\n' "$*" >&2
    exit 1
}

function rerun_in_delegated_scope() {
    if [[ ${!SCOPE_ENV:-0} == "1" ]]; then
        return
    fi
    # systemd-run --user may fail in some environments; fall back to direct run
    if command -v systemd-run >/dev/null 2>&1; then
        if systemd-run --user --scope -p Delegate=yes --quiet \
            env "${SCOPE_ENV}=1" bash "$0" "$@" 2>/dev/null; then
            exit 0
        fi
    fi
    printf '[warn] systemd-run failed, running directly (ensure cpu controller is available)\n' >&2
}

function cleanup() {
    local pid path idx

    printf '\n[cleanup] killing workers...\n' >&2
    for pid in "${WORKER_PIDS[@]}"; do
        kill -9 "${pid}" 2>/dev/null || :
    done
    for pid in "${WORKER_PIDS[@]}"; do
        wait "${pid}" 2>/dev/null || :
    done

    printf '[cleanup] removing cgroups...\n' >&2
    for ((idx = ${#CREATED_CGROUPS[@]} - 1; idx >= 0; idx--)); do
        path=${CREATED_CGROUPS[${idx}]}
        if [[ -d ${path} ]]; then
            rmdir "${path}" 2>/dev/null || :
        fi
    done

    if [[ -n ${CGROUP_ROOT} && -f ${CGROUP_ROOT}/cgroup.subtree_control ]]; then
        printf -- '-cpu\n' >"${CGROUP_ROOT}/cgroup.subtree_control" 2>/dev/null || :
    fi
}

function current_cgroup_dir() {
    local cgroup_path
    cgroup_path=$(sed -n 's|^0::||p' /proc/self/cgroup)
    [[ -n ${cgroup_path} ]] || die "cannot find cgroup v2 path"
    printf '/sys/fs/cgroup%s\n' "${cgroup_path}"
}

function setup_tree() {
    CGROUP_ROOT=$(current_cgroup_dir)
    [[ -f ${CGROUP_ROOT}/cgroup.controllers ]] || die "not running on cgroup v2"
    if ! tr ' ' '\n' <"${CGROUP_ROOT}/cgroup.controllers" | grep -qx cpu; then
        die "cpu controller not available"
    fi
    printf '+cpu\n' >"${CGROUP_ROOT}/cgroup.subtree_control"
}

function new_cgroup() {
    local path="$1"
    mkdir "${path}"
    CREATED_CGROUPS+=("${path}")
}

function move_pid() {
    local path="$1"
    local pid="$2"
    printf '%s\n' "${pid}" >"${path}/cgroup.procs"
}

function spawn_worker() {
    local name="$1"
    local cpu="$2"
    local seconds="$3"
    local pid

    "${WORKER_BIN}" --seconds "${seconds}" --cpu "${cpu}" >/dev/null 2>&1 &
    pid=$!
    WORKER_PIDS+=("${pid}")
    printf '%s' "${pid}"
}

function compile_worker() {
    local compiler=${CC:-cc}
    command -v "${compiler}" >/dev/null 2>&1 || die "compiler not found"
    "${compiler}" -O2 -Wall -Wextra -o "${WORKER_BIN}" "${WORKER_SRC}"
}

function dump_task_groups() {
    printf '\n========== task_group tree ==========\n'
    echo a | sudo -S bpftrace "${SCRIPT_DIR}/dump-task-group.bt" 2>/dev/null | \
        python3 "${SCRIPT_DIR}/dump-task-group.py"
}

function dump_cfs_rq_by_tg() {
    local tg_addr="$1"
    local cpu="$2"
    local cfs_rq_addr

    cfs_rq_addr=$(echo a | sudo -S bpftrace -e "
BEGIN {
    \$tg = (struct task_group*)${tg_addr};
    \$base = (uint64)\$tg->cfs_rq;
    \$cfs_rq = *(uint64*)(\$base + ${cpu} * 8);
    printf(\"%p\\n\", \$cfs_rq);
    exit();
}" 2>/dev/null | grep -E '^0x' | head -1)

    if [[ -z ${cfs_rq_addr} || ${cfs_rq_addr} == "0x0000000000000000" ]]; then
        printf '\n--- cfs_rq for tg=%s CPU=%s: NULL ---\n' "${tg_addr}" "${cpu}"
        return
    fi

    printf '\n--- cfs_rq for tg=%s CPU=%s addr=%s ---\n' "${tg_addr}" "${cpu}" "${cfs_rq_addr}"
    echo a | sudo -S bpftrace "${SCRIPT_DIR}/dump-cfs-rq.bt" "${cfs_rq_addr}" 2>/dev/null | \
        grep -v password | grep -v Attached | grep -v '^BEGIN' | grep -v '^}' | grep -v '^$'
}

function find_tg_by_shares() {
    local target_shares="$1"
    echo a | sudo -S bpftrace -e "
BEGIN {
    \$head = kaddr(\"task_groups\");
    \$pos = *(uint64*)\$head;
    while (\$pos != \$head) {
        \$tg = (struct task_group*)(\$pos - 320);
        if ((uint64)\$tg->shares == ${target_shares}) {
            printf(\"tg=%p shares=%lu parent=%p\n\", \$tg, (uint64)\$tg->shares, \$tg->parent);
        }
        \$pos = *(uint64*)\$pos;
    }
    exit();
}" 2>/dev/null | grep -v password | grep -v Attached | grep -v '^BEGIN' | grep -v '^}' | grep -v '^$'
}

function main() {
    rerun_in_delegated_scope "$@"
    trap cleanup EXIT
    compile_worker
    setup_tree

    # Create hierarchy: A/B, A/C
    local a_path="${CGROUP_ROOT}/demo-A"
    local b_path="${a_path}/demo-B"
    local c_path="${a_path}/demo-C"

    new_cgroup "${a_path}"
    printf '+cpu\n' >"${a_path}/cgroup.subtree_control"
    printf '100\n' >"${a_path}/cpu.weight"

    new_cgroup "${b_path}"
    printf '200\n' >"${b_path}/cpu.weight"

    new_cgroup "${c_path}"
    printf '300\n' >"${c_path}/cpu.weight"

    # Spawn workers:
    #   a -> B, CPU 0
    #   b -> B, CPU 1
    #   c -> C, CPU 0
    #   d -> C, CPU 1
    #   e -> C, CPU 2
    printf '\n[spawn] starting workers...\n' >&2
    local pid_a pid_b pid_c pid_d pid_e
    pid_a=$(spawn_worker a 0 60)
    move_pid "${b_path}" "${pid_a}"
    printf '  a (pid=%s) -> B, CPU 0\n' "${pid_a}"

    pid_b=$(spawn_worker b 1 60)
    move_pid "${b_path}" "${pid_b}"
    printf '  b (pid=%s) -> B, CPU 1\n' "${pid_b}"

    pid_c=$(spawn_worker c 0 60)
    move_pid "${c_path}" "${pid_c}"
    printf '  c (pid=%s) -> C, CPU 0\n' "${pid_c}"

    pid_d=$(spawn_worker d 1 60)
    move_pid "${c_path}" "${pid_d}"
    printf '  d (pid=%s) -> C, CPU 1\n' "${pid_d}"

    pid_e=$(spawn_worker e 2 60)
    move_pid "${c_path}" "${pid_e}"
    printf '  e (pid=%s) -> C, CPU 2\n' "${pid_e}"

    printf '[wait] sleeping 3s for workers to stabilize...\n' >&2
    sleep 3

    # Dump task group tree
    dump_task_groups

    # Find task_group addresses by unique shares
    printf '\n========== Find task_groups by shares ==========\n'
    local tg_a tg_b tg_c
    tg_a=$(find_tg_by_shares 100 | awk '{print $1}' | sed 's/tg=//')
    tg_b=$(find_tg_by_shares 200 | awk '{print $1}' | sed 's/tg=//')
    tg_c=$(find_tg_by_shares 300 | awk '{print $1}' | sed 's/tg=//')

    printf 'A (shares=100) -> %s\n' "${tg_a}"
    printf 'B (shares=200) -> %s\n' "${tg_b}"
    printf 'C (shares=300) -> %s\n' "${tg_c}"

    # Dump cfs_rq for each task_group on relevant CPUs
    for tg in "${tg_a}" "${tg_b}" "${tg_c}"; do
        local name
        case "${tg}" in
            "${tg_a}") name="A (root of demo)" ;;
            "${tg_b}") name="B" ;;
            "${tg_c}") name="C" ;;
        esac
        printf '\n========== Dump %s (tg=%s) ==========\n' "${name}" "${tg}"
        for cpu in 0 1 2; do
            dump_cfs_rq_by_tg "${tg}" "${cpu}"
        done
    done

    printf '\n[done] experiment complete\n' >&2
}

main "$@"
