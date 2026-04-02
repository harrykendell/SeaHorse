#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/_common.sh"

resolve_python
enter_project_dir

print_python
echo
echo "Running Seahorse tests"
"${PYTHON_BIN}" -m pytest tests/python "$@"
