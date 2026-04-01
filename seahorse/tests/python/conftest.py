from __future__ import annotations

import importlib
import importlib.machinery
import importlib.util
import sys
import sysconfig
from pathlib import Path

import pytest

PROJECT_ROOT = Path(__file__).resolve().parents[2]
SRC_PYTHON = PROJECT_ROOT / "src" / "python"

try:
    import seahorse  # type: ignore[import-not-found]  # noqa: F401
except ImportError:
    if str(SRC_PYTHON) not in sys.path:
        sys.path.insert(0, str(SRC_PYTHON))
else:
    import seahorse as _seahorse

    if not hasattr(_seahorse, "__version__"):
        sys.modules.pop("seahorse", None)
        importlib.invalidate_caches()
        if str(SRC_PYTHON) not in sys.path:
            sys.path.insert(0, str(SRC_PYTHON))
        import seahorse  # type: ignore[import-not-found]  # noqa: F401


def _iter_native_candidates() -> list[Path]:
    ext_suffix = sysconfig.get_config_var("EXT_SUFFIX")
    suffixes = [suffix for suffix in (ext_suffix, ".abi3.so", ".pyd") if suffix]
    search_roots = [
        PROJECT_ROOT / "src" / "python" / "seahorse",
        PROJECT_ROOT / "build",
        PROJECT_ROOT / ".venv",
    ]

    matches: list[Path] = []
    for root in search_roots:
        if not root.exists():
            continue
        for suffix in suffixes:
            matches.extend(root.rglob(f"_core*{suffix}"))
    return sorted(set(matches))


@pytest.fixture(scope="session")
def native_core():
    """Load a compiled `_core` module if a compatible artifact exists."""

    import seahorse  # Imported after the source path has been added.

    try:
        return seahorse.api._load_core()
    except RuntimeError:
        pass

    last_error: Exception | None = None
    for path in _iter_native_candidates():
        try:
            spec = importlib.util.spec_from_file_location("seahorse._core", path)
            if spec is None or spec.loader is None:
                continue
            module = importlib.util.module_from_spec(spec)
            sys.modules["seahorse._core"] = module
            spec.loader.exec_module(module)
            return module
        except Exception as exc:  # pragma: no cover - depends on local build state
            last_error = exc
            sys.modules.pop("seahorse._core", None)
            continue

    message = "No compatible compiled seahorse._core artifact was found for this interpreter."
    if last_error is not None:
        message += f" Last load error: {last_error}"
    pytest.skip(message)
