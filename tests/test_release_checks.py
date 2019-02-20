# -*- coding: utf-8 -*-
"""
Performs some pre-release checks
"""
from subprocess import run, PIPE
from re import fullmatch
import pytest


def test_changelog_and_version(accelize_drm):
    """
    Checks if Version match with Git tag and if changelog is up to date.
    """
    if not accelize_drm.pytest_build_environment:
        pytest.skip("Can only be checked in release environment")

    # Ensure tags are pulled
    run(['git', 'fetch', '--tags', '--force'],
        stderr=PIPE, stdout=PIPE, universal_newlines=True)

    # Get head tag if any
    result = run(['git', 'describe', '--abbrev=0', '--exact-match', '--tags',
                  'HEAD'], stderr=PIPE, stdout=PIPE, universal_newlines=True)

    if not result.returncode:
        pytest.skip("No tag on head to check")

    tag = result.stdout.strip()

    # Checks tag format using library version
    lib_ver = accelize_drm.get_api_version()
    assert tag == f"v{lib_ver.major}.{lib_ver.minor}.{lib_ver.revision}"

    # Check if changelog is up-to-date

    if '-' in lib_ver.revision:
        # Pre-releases don't need up to date changelog
        return

    changelog_path = f'{accelize_drm.pytest_build_source_dir}/CHANGELOG'
    with open(changelog_path, 'wt') as changelog:
        last_change = changelog.readline().strip()

    assert fullmatch(
        r"\* [a-zA-Z]{3} [a-zA-Z]{3} [0-9]{2} [0-9]{4} Accelize " + tag,
        last_change)
