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
        pytest.skip("Can only be checked in build environment")

    # Ensure tags are pulled
    run(['git', 'fetch', '--tags', '--force'],
        stderr=PIPE, stdout=PIPE, universal_newlines=True)

    # Get head tag if any
    result = run(['git', 'describe', '--abbrev=0', '--exact-match', '--tags',
                  'HEAD'], stderr=PIPE, stdout=PIPE, universal_newlines=True)

    if result.returncode:
        pytest.skip("Can only be checked on tagged git head")

    tag = result.stdout.strip()
    version = tag.lstrip('v')

    # Checks tag format using library version
    lib_ver = accelize_drm.get_api_version()
    assert tag == f"v{lib_ver.version.split('+')[0]}"

    # Check tag format match semantic versioning

    if not fullmatch(r'^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)'
                     r'(-(0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)'
                     r'(\.(0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*)?'
                     r'(\+[0-9a-zA-Z-]+(\.[0-9a-zA-Z-]+)*)?$', version):
        pytest.fail(f'"{version}" does not match semantic versioning format.')

    # Check if changelog is up-to-date (Not for prereleases)
    if not lib_ver.prerelease:
        changelog_path = f'{accelize_drm.pytest_build_source_dir}/CHANGELOG'
        with open(changelog_path, 'rt') as changelog:
            last_change = changelog.readline().strip()

        assert fullmatch(
            r"\* [a-zA-Z]{3} [a-zA-Z]{3} [0-9]{2} [0-9]{4} Accelize " + tag,
            last_change)
