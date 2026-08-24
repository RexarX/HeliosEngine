#!/usr/bin/env bash

set -u -o pipefail

# ---- Locate the test binary --------------------------------------------
DEFAULT_BIN_CANDIDATES=(
  "bin/tests/debug-macos-arm64/helios_memory_tests"
  "bin/tests/relwithdebinfo-macos-arm64/helios_memory_tests"
  "bin/tests/release-macos-arm64/helios_memory_tests"
)

TEST_BIN="${1:-${HELIOS_TEST_BIN:-}}"
ITERATIONS="${2:-${HELIOS_ITERATIONS:-50}}"
ITER_TIMEOUT="${3:-${HELIOS_ITER_TIMEOUT:-10}}"

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

echo "== macos_tests.sh: TemporaryStorage isolation run =="
echo "binary:             ${TEST_BIN}"
echo "iterations:         ${ITERATIONS}"
echo "per-iter timeout:   ${ITER_TIMEOUT}s"
echo

# ---- timeout shim with stack-sample capture -----------------------------
# Deliberately does NOT delegate to gtimeout/timeout: those just SIGTERM/
# SIGKILL on expiry, which tells you *that* something hung but not *where*.
# For a livelock we specifically want per-thread stacks at the moment of
# the hang, so we always drive this ourselves and call `sample` (ships
# with macOS/Xcode CLT, no lldb entitlements required) right before the
# kill. On timeout, a stack sample is left at $SAMPLE_OUT for inspection
# or CI upload; on a clean run it's removed.
SAMPLE_OUT="/tmp/macos_tests_sample.txt"

run_with_timeout() {
  local secs="$1"; shift
  rm -f "${SAMPLE_OUT}"

  "$@" &
  local pid=$!

  (
    sleep "${secs}"
    if kill -0 "${pid}" 2>/dev/null; then
      # Capture all-thread stacks for every process in the tree (the
      # binary + any threads it spawned) before killing. `sample` takes
      # one snapshot over ~1s; that's enough to see which threads are
      # parked in the ResetAll/ThreadBinding CAS loops vs. blocked
      # elsewhere.
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

# ---- Phase 1: quick smoke of the whole TemporaryStorage suite ----------
echo "-- Phase 1: full TemporaryStorage suite (single pass, fast subcases) --"
if ! run_with_timeout 30 "${TEST_BIN}" \
    --test-suite="helios::mem::TemporaryStorage" \
    --no-skip; then
  echo "FAIL: TemporaryStorage suite failed or hung on a fast pass." >&2
  exit 1
fi
echo "ok"
echo

# ---- Phase 2: repeat the racy stress subcases N times -------------------
# These are the two subcases that actually pit ResetAll() against
# ThreadBinding::~ThreadBinding() under load; everything else in the
# suite is comparatively low-risk.
RACY_SUBCASES=(
  "ResetAll is safe while other threads exit"
  "ResetAll racing thread exit does not stick slots"
)

fail=0
for subcase in "${RACY_SUBCASES[@]}"; do
  echo "-- Phase 2: stressing subcase: '${subcase}' --"
  for ((i = 1; i <= ITERATIONS; i++)); do
    if ! run_with_timeout "${ITER_TIMEOUT}" "${TEST_BIN}" \
        --test-suite="helios::mem::TemporaryStorage" \
        --subcase="${subcase}" \
        --no-skip >/tmp/macos_tests_iter.log 2>&1; then
      echo
      echo "FAIL: iteration ${i}/${ITERATIONS} of '${subcase}' hung or failed" >&2
      echo "      (treat this as a reproducer — see log/stacks below)" >&2
      echo "---- last output ----" >&2
      cat /tmp/macos_tests_iter.log >&2
      echo "----------------------" >&2
      if [[ -s "${SAMPLE_OUT}" ]]; then
        echo "---- thread stacks at hang (sample) ----" >&2
        cat "${SAMPLE_OUT}" >&2
        echo "-----------------------------------------" >&2
        # Preserve for CI artifact upload; a fresh run overwrites this.
        cp "${SAMPLE_OUT}" "./macos_tests_hang_sample_$(date +%s).txt" 2>/dev/null
      else
        echo "note: no stack sample captured (process may have exited on" >&2
        echo "      its own with a non-zero status rather than hanging —" >&2
        echo "      check the doctest output above for an assertion failure)." >&2
      fi
      fail=1
      break
    fi
    printf "\r  iteration %d/%d ok" "${i}" "${ITERATIONS}"
  done
  echo
  if [[ "${fail}" -eq 1 ]]; then
    break
  fi
  echo "  all ${ITERATIONS} iterations passed"
  echo
done

rm -f /tmp/macos_tests_iter.log "${SAMPLE_OUT}"

if [[ "${fail}" -ne 0 ]]; then
  echo
  echo "== RESULT: reproduced a TemporaryStorage hang/failure in isolation ==" >&2
  exit 1
fi

echo "== RESULT: TemporaryStorage held up across ${ITERATIONS} stress iterations per subcase =="
exit 0
