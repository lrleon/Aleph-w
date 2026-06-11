"""
Shared pytest fixtures for the aleph_ca test suite.
"""

import os
import sys

import pytest


@pytest.fixture(scope="session", autouse=True)
def _ensure_aleph_ca_importable():
    """Fail fast with a clear error if the extension isn't installed.

    When running from a fresh tree without `pip install -e python/`, the
    failure mode is opaque ("ModuleNotFoundError: _core"). This fixture
    surfaces a friendlier message pointing the user at the right command.
    """
    try:
        import aleph_ca  # noqa: F401
    except ImportError as exc:  # pragma: no cover - environment plumbing
        pytest.exit(
            "aleph_ca is not importable: "
            f"{exc}. Run `pip install --editable python/[test]` first."
        )