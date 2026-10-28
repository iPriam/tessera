#!/bin/bash
# Normalize the cross target before distcc examines the compiler arguments.
set -euo pipefail
export PATH=/root/.kira/toolchains/llvm/23.1.0/x86_64-linux-gnu/bin:$PATH
export DISTCC_HOSTS="${DISTCC_HOSTS:-localhost/3 100.85.181.198/8}"
export DISTCC_FALLBACK=0
compiler=$(basename "$1")
shift
args=()
depfile=
next_is_depfile=false
for arg in "$@"; do
    if "$next_is_depfile"; then
        depfile=$arg
        next_is_depfile=false
    fi
    case "$arg" in
        --target=*) args+=(-target "${arg#--target=}") ;;
        -MF) next_is_depfile=true; args+=("$arg") ;;
        -MF*) depfile=${arg#-MF}; args+=("$arg") ;;
        *) args+=("$arg") ;;
    esac
done
distcc "$compiler" "${args[@]}"
if test -n "$depfile"; then
    python3 "$(dirname "$0")/normalize-depfile.py" "$depfile"
fi
