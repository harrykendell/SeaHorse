#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

PYTHON_BIN="${PYTHON_BIN:-}"
CONDA_ENV_NAME=""
INSTALL_MODE="editable"
WHEEL_DIR="${PROJECT_DIR}/dist"
SMOKE_TEST=1
INSTALL_NO_DEPS=""
CONDA_BIN="${CONDA_EXE:-}"

warn() {
  echo "Warning: $*" >&2
}

die() {
  echo "Error: $*" >&2
  exit 1
}

if [[ -z "${CONDA_BIN}" ]] && command -v conda >/dev/null 2>&1; then
  CONDA_BIN="$(command -v conda)"
fi

describe_python_target() {
  local python_bin="$1"
  "${python_bin}" - <<'PY'
import os
import site
import sys
import sysconfig

purelib = sysconfig.get_paths().get("purelib") or ""
usersite = site.getusersitepackages() if site.ENABLE_USER_SITE else ""
is_writable = int(bool(purelib) and os.access(purelib, os.W_OK))

print(f"executable={sys.executable}")
print(f"version={sys.version.split()[0]}")
print(f"prefix={sys.prefix}")
print(f"purelib={purelib}")
print(f"usersite={usersite}")
print(f"purelib_writable={is_writable}")
PY
}

usage() {
  cat <<'EOF'
Install Seahorse into a chosen Python environment.

Usage:
  scripts/build_and_install_local.sh [options]

Options:
  --editable              Install with `pip install -e`. This is the default.
  --wheel                 Build a wheel, then install that wheel.
  --python PATH           Python interpreter to use explicitly.
  --conda-env NAME        Resolve Python from a Conda environment without activating it first.
  --wheel-dir PATH        Where to write the built wheel in `--wheel` mode.
  --no-deps               Install with --no-deps.
  --skip-smoke-test       Skip the import smoke test after installation.
  -h, --help              Show this help text.

Examples:
  scripts/build_and_install_local.sh
  scripts/build_and_install_local.sh --editable
  scripts/build_and_install_local.sh --wheel
  scripts/build_and_install_local.sh --python .venv/bin/python
  scripts/build_and_install_local.sh --conda-env my-env
  scripts/build_and_install_local.sh --python python3 --wheel --no-deps
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --editable)
      INSTALL_MODE="editable"
      shift
      ;;
    --wheel)
      INSTALL_MODE="wheel"
      shift
      ;;
    --python)
      PYTHON_BIN="$2"
      CONDA_ENV_NAME=""
      shift 2
      ;;
    --conda-env)
      CONDA_ENV_NAME="$2"
      shift 2
      ;;
    --wheel-dir)
      WHEEL_DIR="$2"
      shift 2
      ;;
    --no-deps)
      INSTALL_NO_DEPS="1"
      shift
      ;;
    --skip-smoke-test)
      SMOKE_TEST=0
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      echo >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ -z "${PYTHON_BIN}" && -z "${CONDA_ENV_NAME}" ]]; then
  if [[ -n "${CONDA_PREFIX:-}" ]]; then
    if [[ -x "${CONDA_PREFIX}/bin/python" ]]; then
      PYTHON_BIN="${CONDA_PREFIX}/bin/python"
    else
      warn "CONDA_PREFIX is set to '${CONDA_PREFIX}', but '${CONDA_PREFIX}/bin/python' is not executable."
      if [[ -n "${CONDA_DEFAULT_ENV:-}" ]]; then
        warn "Conda claims '${CONDA_DEFAULT_ENV}' is active, but that activation looks stale."
      fi
      if [[ -n "${CONDA_EXE:-}" ]]; then
        warn "CONDA_EXE points to '${CONDA_EXE}'."
      fi
      if [[ -x "${HOME}/miniforge3/bin/python" ]]; then
        warn "Detected Miniforge at '${HOME}/miniforge3'."
        warn "Try: source \"${HOME}/miniforge3/etc/profile.d/conda.sh\" && conda activate base"
        warn "Or rerun this script with: --python \"${HOME}/miniforge3/bin/python\""
        warn "Or rerun this script with: --conda-env base"
      fi
      die "Refusing to guess a fallback interpreter while Conda state is inconsistent."
    fi
  elif command -v python >/dev/null 2>&1; then
    PYTHON_BIN="python"
  elif command -v python3 >/dev/null 2>&1; then
    PYTHON_BIN="python3"
    warn "No active Conda environment and no 'python' executable were found on PATH."
    warn "Falling back to 'python3' ($(command -v python3))."
    if [[ -n "${CONDA_BIN}" && -x "${CONDA_BIN}" ]]; then
      warn "If you intended a Conda environment, rerun with '--conda-env NAME' or activate one first."
    fi
  else
    die "Could not find 'python' or 'python3' on PATH."
  fi
fi

if [[ -n "${CONDA_ENV_NAME}" ]]; then
  if [[ -z "${CONDA_BIN}" || ! -x "${CONDA_BIN}" ]]; then
    die "Could not find a usable 'conda' executable, but --conda-env was requested."
  fi

  PYTHON_BIN="$("${CONDA_BIN}" run -n "${CONDA_ENV_NAME}" python -c 'import sys; print(sys.executable)')"
  if [[ -z "${PYTHON_BIN}" ]]; then
    die "Failed to resolve Python for Conda environment '${CONDA_ENV_NAME}'."
  fi
fi

mkdir -p "${WHEEL_DIR}"

echo "Using Python: ${PYTHON_BIN}"
"${PYTHON_BIN}" --version

PYTHON_EXE=""
PYTHON_VERSION=""
PYTHON_PREFIX=""
PYTHON_PURELIB=""
PYTHON_USERSITE=""
PYTHON_PURELIB_WRITABLE="0"

while IFS='=' read -r key value; do
  case "${key}" in
    executable) PYTHON_EXE="${value}" ;;
    version) PYTHON_VERSION="${value}" ;;
    prefix) PYTHON_PREFIX="${value}" ;;
    purelib) PYTHON_PURELIB="${value}" ;;
    usersite) PYTHON_USERSITE="${value}" ;;
    purelib_writable) PYTHON_PURELIB_WRITABLE="${value}" ;;
  esac
done < <(describe_python_target "${PYTHON_BIN}")

echo "Resolved interpreter: ${PYTHON_EXE}"
echo "Interpreter prefix: ${PYTHON_PREFIX}"
echo "Global site-packages: ${PYTHON_PURELIB}"
if [[ -n "${PYTHON_USERSITE}" ]]; then
  echo "User site-packages: ${PYTHON_USERSITE}"
fi

if [[ "${PYTHON_PURELIB_WRITABLE}" != "1" ]]; then
  warn "The selected interpreter cannot write to its normal site-packages directory."
  if [[ -n "${PYTHON_USERSITE}" ]]; then
    warn "pip will likely fall back to the user site at '${PYTHON_USERSITE}'."
  fi
  warn "Use '--python PATH' or '--conda-env NAME' if you intended a different environment."
fi

INSTALL_ARGS=(--force-reinstall)
if [[ -n "${INSTALL_NO_DEPS}" ]]; then
  INSTALL_ARGS+=(--no-deps)
elif "${PYTHON_BIN}" -c "import numpy" >/dev/null 2>&1; then
  INSTALL_ARGS+=(--no-deps)
  echo
  echo "Detected existing NumPy in the target environment; installing with --no-deps."
fi

INSTALL_TARGET=""
WHEEL_PATH=""

if [[ "${INSTALL_MODE}" == "wheel" ]]; then
  mkdir -p "${WHEEL_DIR}"

  echo
  echo "Building wheel into ${WHEEL_DIR}"
  "${PYTHON_BIN}" -m pip wheel "${PROJECT_DIR}" --wheel-dir "${WHEEL_DIR}" --no-deps

  WHEEL_PATH="$(ls -1t "${WHEEL_DIR}"/seahorse-*.whl | head -n 1)"
  if [[ -z "${WHEEL_PATH}" ]]; then
    die "Failed to locate a built wheel in ${WHEEL_DIR}"
  fi

  INSTALL_TARGET="${WHEEL_PATH}"
  echo
  echo "Installing wheel ${WHEEL_PATH}"
  "${PYTHON_BIN}" -m pip install "${INSTALL_ARGS[@]}" "${INSTALL_TARGET}"
else
  INSTALL_TARGET="${PROJECT_DIR}"
  echo
  echo "Installing editable package from ${PROJECT_DIR}"
  "${PYTHON_BIN}" -m pip install "${INSTALL_ARGS[@]}" -e "${INSTALL_TARGET}"
fi

if [[ "${SMOKE_TEST}" -eq 1 ]]; then
  echo
  echo "Running import smoke test"
  "${PYTHON_BIN}" - <<'PY'
import seahorse
import seahorse._core as core

grid = core.Grid1D(8, -1.0, 1.0)
print("seahorse version:", seahorse.__version__)
print("grid dim:", grid.dim)
print("x shape:", grid.x.shape)
PY
fi

echo
echo "Done."
if [[ "${INSTALL_MODE}" == "wheel" ]]; then
  echo "Installed wheel: ${WHEEL_PATH}"
else
  echo "Installed editable package from: ${PROJECT_DIR}"
fi
