#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/_common.sh"

resolve_python
configure_build_frontend
enter_project_dir

print_python
mkdir -p dist

echo
echo "Building Seahorse wheel into ${PROJECT_DIR}/dist"
"${PYTHON_BIN}" -m pip wheel . --wheel-dir dist --no-deps "${BUILD_FRONTEND_ARGS[@]}"

WHEEL_PATH="$(ls -1t dist/seahorse-*.whl | head -n 1)"
if [[ -z "${WHEEL_PATH}" ]]; then
  echo "Error: Failed to locate a built wheel in dist/." >&2
  exit 1
fi

echo
echo "Built wheel: ${WHEEL_PATH}"
echo "Install it elsewhere with:"
echo "  pip install ${WHEEL_PATH}"
