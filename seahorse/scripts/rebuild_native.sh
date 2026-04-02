#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/_common.sh"

resolve_python
configure_build_frontend
enter_project_dir

print_python
echo
echo "Rebuilding Seahorse native extension in editable mode"
"${PYTHON_BIN}" -m pip install --force-reinstall --no-deps "${BUILD_FRONTEND_ARGS[@]}" -e .

echo
echo "Running import smoke test"
run_smoke_test

echo
echo "Native rebuild completed."
echo "Run ./scripts/test.sh to verify the full suite."
