"""Launch the Helios module tool without setting PYTHONPATH."""

from __future__ import annotations

import sys
from pathlib import Path

_PACKAGE_ROOT = Path(__file__).resolve().parent
if str(_PACKAGE_ROOT) not in sys.path:
    sys.path.insert(0, str(_PACKAGE_ROOT))

from helios_modules.__main__ import main  # noqa: E402

if __name__ == "__main__":
    if sys.version_info < (3, 11):
        print("error: Python 3.11 or newer is required", file=sys.stderr)
        raise SystemExit(1)
    raise SystemExit(main())
