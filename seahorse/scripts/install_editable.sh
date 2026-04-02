#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/_common.sh"

resolve_python
configure_build_frontend
enter_project_dir

print_python
echo
echo "Installing Seahorse in editable mode from ${PROJECT_DIR}"
"${PYTHON_BIN}" -m pip install "${BUILD_FRONTEND_ARGS[@]}" -e ".[dev]"

echo
echo "Running import smoke test"
run_smoke_test

echo
echo "Editable install is ready."
echo "Python changes are live from this checkout."
echo "After C++ changes, run ./scripts/rebuild_native.sh and restart long-running Python processes."
