#!/bin/sh
# Regression tests for Focl control flow.
#
# Each test/control/*.focl is executed and checked against:
#   *.expected  exact stdout (ANSI colour codes stripped)
#   *.contains  a substring that must appear in the output
#   *.exit      expected process exit code (default 0)
# If neither .expected nor .contains exists, the output must be empty.

set -u

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT" || exit 1

if [ -n "${FOCL:-}" ]; then
    :
elif [ -x ./build/focl ]; then
    FOCL=./build/focl
elif [ -x ./build/focl.exe ]; then
    FOCL=./build/focl.exe
elif [ -x ./focl ]; then
    FOCL=./focl
elif [ -x ./focl.exe ]; then
    FOCL=./focl.exe
else
    echo "error: focl binary not found; run make first" >&2
    exit 1
fi

TMPDIR=$(mktemp -d 2>/dev/null || mktemp -d -t focl)
trap 'rm -rf "$TMPDIR"' EXIT INT TERM

ESC=$(printf '\033')
strip_ansi() {
    sed "s/${ESC}\\[[0-9;]*m//g"
}

total=0
failed=0

for file in test/control/*.focl; do
    [ -f "$file" ] || continue
    base=${file%.focl}
    name=${file#test/control/}
    name=${name%.focl}
    total=$((total + 1))

    out_file="$TMPDIR/out"
    "$FOCL" "$file" >"$out_file" 2>&1
    code=$?
    out=$(strip_ansi <"$out_file")

    want_exit=0
    if [ -f "$base.exit" ]; then
        want_exit=$(cat "$base.exit")
    fi

    reason=""
    if [ "$code" -ne "$want_exit" ]; then
        reason="exit code $code, expected $want_exit"
    fi

    if [ -f "$base.expected" ]; then
        expected=$(cat "$base.expected")
        if [ "$out" != "$expected" ]; then
            reason="${reason:+$reason; }stdout mismatch"
        fi
    elif [ -f "$base.contains" ]; then
        needle=$(cat "$base.contains")
        if ! printf '%s' "$out" | grep -Fq "$needle"; then
            reason="${reason:+$reason; }output does not contain: $needle"
        fi
    else
        if [ -n "$out" ]; then
            reason="${reason:+$reason; }expected no output"
        fi
    fi

    if [ -n "$reason" ]; then
        failed=$((failed + 1))
        echo "FAIL $name: $reason"
        if [ -n "$out" ]; then
            echo "----- output -----"
            printf '%s\n' "$out"
            echo "------------------"
        fi
    else
        echo "PASS $name"
    fi
done

echo
if [ "$failed" -ne 0 ]; then
    echo "$failed/$total control-flow tests failed"
    exit 1
fi
echo "all $total control-flow tests passed"
