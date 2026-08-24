#!/usr/bin/env bash

set -u -o pipefail

# ---- Locate the test binary --------------------------------------------
DEFAULT_BIN_CANDIDATES=(
  "bin/tests/debug-darwin-arm64/helios_memory_tests"
  "bin/tests/relwithdebinfo-darwin-arm64/helios_memory_tests"
  "bin/tests/release-darwin-arm64/helios_memory_tests"
)

TEST_BIN="${1:-${HELIOS_TEST_BIN:-}}"
ITERATIONS="${2:-${HELIOS_ITERATIONS:-50}}"
ITER_TIMEOUT="${3:-${HELIOS_ITER_TIMEOUT:-10}}"
SWEEP_TIMEOUT="${HELIOS_SWEEP_TIMEOUT:-20}"

if [[ -z "${TEST_BIN}" ]]; then
  for candidate in "${DEFAULT_BIN_CANDIDATES[@]}"; do
    if [[ -x "${candidate}" ]]; then
      TEST_BIN="${candidate}"
      break
    fi
  done
fi

if [[ -z "${TEST_BIN}" || ! -x "${TEST_BIN}" ]]; then
  echo "error: could not find helios_memory_tests binary." >&2
  echo "  tried: ${DEFAULT_BIN_CANDIDATES[*]}" >&2
  echo "  pass an explicit path as the first argument, or set HELIOS_TEST_BIN." >&2
  exit 2
fi

echo "== macos_tests.sh: helios_memory_tests bisection run =="
echo "binary:             ${TEST_BIN}"
echo "sweep timeout:      ${SWEEP_TIMEOUT}s per test case"
echo "stress iterations:  ${ITERATIONS} (if a culprit is found)"
echo "stress timeout:     ${ITER_TIMEOUT}s per iteration"
echo

# ---- timeout shim with stack-sample capture -----------------------------
# Deliberately does NOT delegate to gtimeout/timeout: those just SIGTERM/
# SIGKILL on expiry, which tells you *that* something hung but not
# *where*. We want per-thread stacks at the moment of the hang, so we
# drive this ourselves and call `sample` (ships with macOS/Xcode CLT, no
# lldb entitlements required) right before the kill. On timeout, a stack
# sample is left at $SAMPLE_OUT; on a clean run it's removed.
SAMPLE_OUT="/tmp/macos_tests_sample.txt"

run_with_timeout() {
  local secs="$1"; shift
  rm -f "${SAMPLE_OUT}"

  "$@" &
  local pid=$!

  (
    sleep "${secs}"
    if kill -0 "${pid}" 2>/dev/null; then
      # One ~1s snapshot of every thread's stack before killing — enough
      # to see exactly which function each thread is parked in.
      sample "${pid}" 1 -mayDie -file "${SAMPLE_OUT}" >/dev/null 2>&1
      kill -9 "${pid}" 2>/dev/null
    fi
  ) &
  local watchdog=$!

  wait "${pid}"
  local status=$?
  kill "${watchdog}" 2>/dev/null
  wait "${watchdog}" 2>/dev/null
  return "${status}"
}

report_hang() {
  local label="$1" logfile="$2"
  echo
  echo "FAIL: ${label} hung or failed" >&2
  echo "---- last output ----" >&2
  cat "${logfile}" >&2
  echo "----------------------" >&2
  if [[ -s "${SAMPLE_OUT}" ]]; then
    echo "---- thread stacks at hang (sample) ----" >&2
    cat "${SAMPLE_OUT}" >&2
    echo "-----------------------------------------" >&2
    cp "${SAMPLE_OUT}" "./macos_tests_hang_sample_$(date +%s).txt" 2>/dev/null
  else
    echo "note: no stack sample captured (process may have exited on its" >&2
    echo "      own with a non-zero status rather than hanging — check" >&2
    echo "      the doctest output above for an assertion failure)." >&2
  fi
}

# ---- Phase 0: bisect the whole binary at TEST_CASE granularity ---------
echo "-- Phase 0: enumerating TEST_CASEs --"
# Avoid `mapfile`/`readarray` — macOS ships bash 3.2 by default, which
# doesn't have them.
ALL_CASES=()
while IFS= read -r line; do
  [[ -n "${line}" ]] && ALL_CASES+=("${line}")
done < <("${TEST_BIN}" --list-test-cases 2>/dev/null | grep -v '^\[doctest\]')

if [[ "${#ALL_CASES[@]}" -eq 0 ]]; then
  echo "error: --list-test-cases returned no test case names; binary or" >&2
  echo "       doctest output format may have changed." >&2
  exit 2
fi
echo "found ${#ALL_CASES[@]} test case(s)"
echo

CULPRIT=""
for ((idx = 0; idx < ${#ALL_CASES[@]}; idx++)); do
  case_name="${ALL_CASES[$idx]}"
  printf "  [%d/%d] %s ... " "$((idx + 1))" "${#ALL_CASES[@]}" "${case_name}"
  if ! run_with_timeout "${SWEEP_TIMEOUT}" "${TEST_BIN}" \
      --test-case="${case_name}" \
      --no-skip >/tmp/macos_tests_sweep.log 2>&1; then
    echo "HUNG/FAILED"
    report_hang "TEST_CASE '${case_name}' (run in isolation, case ${idx} of ${#ALL_CASES[@]})" \
        /tmp/macos_tests_sweep.log
    CULPRIT="${case_name}"
    break
  fi
  echo "ok"
done
rm -f /tmp/macos_tests_sweep.log

if [[ -z "${CULPRIT}" ]]; then
  echo
  echo "== Phase 0: every TEST_CASE passed in isolation. =="
  echo "The hang under ctest did not reproduce running each TEST_CASE"
  echo "alone — this points at STATE ACCUMULATED ACROSS test cases in a"
  echo "single process (order-dependent), not a self-contained hang in"
  echo "any one case. Next step: re-run this binary's full default"
  echo "invocation (no filters) under the same timeout to confirm it"
  echo "reproduces, then bisect by running growing PREFIXES of the"
  echo "${#ALL_CASES[@]} listed cases (in list order) until a prefix"
  echo "first hangs — that identifies which earlier case leaves behind"
  echo "the state that triggers it."
  echo
  echo "-- Phase 0b: full default run (no filters), single pass --"
  if ! run_with_timeout "${SWEEP_TIMEOUT}" "${TEST_BIN}" --no-skip \
      >/tmp/macos_tests_full.log 2>&1; then
    report_hang "full unfiltered binary run" /tmp/macos_tests_full.log
    echo
    echo "== RESULT: confirmed order-dependent hang; not isolated to a single TEST_CASE. ==" >&2
    rm -f /tmp/macos_tests_full.log
    exit 1
  fi
  rm -f /tmp/macos_tests_full.log
  echo "ok — did not reproduce even in a single full unfiltered pass."
  echo "This may be a rarer race; consider looping Phase 0b with"
  echo "HELIOS_SWEEP_TIMEOUT raised, or run ctest itself a few more times."
  exit 0
fi

echo
echo "== Phase 0 found a culprit: '${CULPRIT}' =="
echo

# ---- Phase 1: quick smoke of the culprit's suite, if it's TemporaryStorage
if [[ "${CULPRIT}" == *TemporaryStorage* || "${CULPRIT}" == *ResetAll* ]]; then
  echo "-- Phase 1: full TemporaryStorage suite (single pass) --"
  if ! run_with_timeout 30 "${TEST_BIN}" \
      --test-suite="helios::mem::TemporaryStorage" \
      --no-skip >/tmp/macos_tests_p1.log 2>&1; then
    report_hang "TemporaryStorage suite (fast pass, post-culprit-discovery)" /tmp/macos_tests_p1.log
    rm -f /tmp/macos_tests_p1.log
    exit 1
  fi
  rm -f /tmp/macos_tests_p1.log
  echo "ok"
  echo
fi

# ---- Phase 2: repeat the exact culprit TEST_CASE N times, standalone --
echo "-- Phase 2: stressing '${CULPRIT}' in isolation (fresh process each time) --"
fail=0
for ((i = 1; i <= ITERATIONS; i++)); do
  if ! run_with_timeout "${ITER_TIMEOUT}" "${TEST_BIN}" \
      --test-case="${CULPRIT}" \
      --no-skip >/tmp/macos_tests_iter.log 2>&1; then
    report_hang "iteration ${i}/${ITERATIONS} of '${CULPRIT}'" /tmp/macos_tests_iter.log
    fail=1
    break
  fi
  printf "\r  iteration %d/%d ok" "${i}" "${ITERATIONS}"
done
echo
rm -f /tmp/macos_tests_iter.log "${SAMPLE_OUT}"

if [[ "${fail}" -ne 0 ]]; then
  echo
  echo "== RESULT: '${CULPRIT}' reproduces standalone — self-contained hang. ==" >&2
  exit 1
fi

echo
echo "== RESULT: '${CULPRIT}' hung only as part of the full sweep, not standalone. =="
echo "This points at cross-test state leakage (e.g. TemporaryStorage's"
echo "static g_slots pool, or leftover threads from an earlier case)"
echo "rather than a bug confined to this test case's own body."
exit 1
