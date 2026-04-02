#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PYTHON_BIN="${PYTHON:-python3}"
BUILD_FRONTEND_ARGS=()

resolve_python() {
  if [[ -x "${PYTHON_BIN}" ]]; then
    :
  elif command -v "${PYTHON_BIN}" >/dev/null 2>&1; then
    PYTHON_BIN="$(command -v "${PYTHON_BIN}")"
  else
    echo "Error: Could not find Python interpreter '${PYTHON_BIN}'." >&2
    echo "Set PYTHON=/path/to/python if you want to use a different environment." >&2
    exit 1
  fi

  export PATH="$(dirname "${PYTHON_BIN}"):${PATH}"
}

enter_project_dir() {
  cd "${PROJECT_DIR}"
}

configure_build_frontend() {
  BUILD_FRONTEND_ARGS=()
  if "${PYTHON_BIN}" -c "import nanobind, scikit_build_core" >/dev/null 2>&1 && cmake --version >/dev/null 2>&1; then
    BUILD_FRONTEND_ARGS+=(--no-build-isolation)
  fi
}

print_python() {
  echo "Using Python: $("${PYTHON_BIN}" -c 'import sys; print(sys.executable)')"
  "${PYTHON_BIN}" --version
}

run_smoke_test() {
  "${PYTHON_BIN}" - <<'PY'
import os
import sys
from datetime import datetime

import seahorse
import seahorse._core as core

grid = core.Grid1D(8, -1.0, 1.0)
print("python:", sys.executable)
print("seahorse package:", getattr(seahorse, "__file__", "<namespace package>"))
print("native core:", getattr(core, "__file__", "<unknown>"))
print("native core mtime:", datetime.fromtimestamp(os.path.getmtime(core.__file__)).isoformat())
print("grid dim:", grid.dim)
print("x shape:", grid.x.shape)
PY
}
