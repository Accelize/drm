# -*- coding: utf-8 -*-
"""
Test ABI/API compatibility
"""
import os
import re

import pytest
from tests.conftest import perform_once

LIB_NAMES = ('libaccelize_drmc', 'libaccelize_drm')
REPOSITORY_PATH = 'https://github.com/Accelize/drmlib'


def _run(*command, **kwargs):
    """
    Run a command.

    Args:
        command (list of str):

    Returns:
        subprocess.CompletedProcess
    """
    from subprocess import run, CalledProcessError, PIPE

    result = run(*command, stdout=PIPE, stderr=PIPE,
                 universal_newlines=True, **kwargs)
    try:
        result.check_returncode()
    except CalledProcessError as exception:
        print(exception.stdout, exception.stderr, sep='\n')
    return result


def make(path, target=None):
    """
    Make.

    Args:
        path (str): Path of sources to make.
        target (str): Target
    """
    _run(['make', '-j', str(os.cpu_count())] + ([target] if target else []),
         cwd=path)


def dump_abi(dump_file, so_file, include, version, name):
    """
    Dump library ABI.

    Args:
        dump_file (str): Path to target ".abidump" file
        so_file (str): Path to source "libaccelize_drm.so" or
            "libaccelize_drmc.so" file
        include (str): Path to public headers "include" directory.
        version (str): Library version
        name (str): Library name.

    Returns:
        tuple of str: version and dump_file
    """
    _run(['abi-dumper', so_file, '-public-headers', include, '-o', dump_file,
          '-lver', version])
    return version, dump_file, name


def checks_abi_compliance(old_dump, new_dump, name, report_path):
    """
    Checks ABI compliance.

    Args:
        old_dump (str): Path to old library version dump.
        new_dump (str): Path to new library version dump.
        name (str): Library name.
        report_path (str): HTML report path.

    Returns:
        str: Report
    """
    return _run(['abi-compliance-checker', '-l', name, '-old', old_dump, '-new',
                 new_dump, '-report-path', report_path]).stdout


def make_tag(version, path):
    """
    Clone and make DRMlib previous versions sources

    Args:
        version (str): version to clone.
        path (str): Path to target directory.

    Returns:
        tuple of str: version and path
    """
    # Clone sources
    os.mkdir(path)
    _run(['git', 'clone', '-q', '-b', 'v%s' % version, '--depth', '1',
          REPOSITORY_PATH, '%s/src' % path])

    # Make
    _run(['cmake', '-DCMAKE_BUILD_TYPE=Debug', './src'], cwd=path)
    make(path, 'all')

    return version, path


def get_reference_versions(tmpdir, abi_version):
    """
    Get reference versions (Same ABI, ignoring "patch" versions).

    Args:
        tmpdir: pytest tmpdir from test
        abi_version: (str): ABI version.

    Returns:
        dict: version, directory
    """
    tags = _run(['git', 'ls-remote', '--tags', REPOSITORY_PATH]).stdout
    versions = {
        tag.group(1) : str(tmpdir.join(tag.group(1))) for tag in list(map(
                lambda x: re.search(r'v((\d+)\.\d+\.\d+)$', x), tags.splitlines()))
            if (tag and int(tag.group(2))==abi_version) }
    return versions


def test_abi_compliance(tmpdir, accelize_drm):
    """
    Test the ABI/API compliance of the lib_name.
    """
    perform_once(__name__ + '.test_abi_compliance')

    if not accelize_drm.pytest_build_environment:
        pytest.skip('This test is only performed on build environment.')
    elif not accelize_drm.pytest_build_type == 'debug':
        pytest.xfail('This test needs libraries compiled in debug mode.')

    # Initialize test
    from concurrent.futures import ThreadPoolExecutor, as_completed
    build_futures = []
    dump_futures = []
    latest_futures = []
    tools_futures = []
    latest_dumps = {}
    reports = {}

    with ThreadPoolExecutor() as executor:

        def dumps_library(lib_version, lib_path, futures):
            """
            Dumps a version asynchronously.

            Args:
                lib_version (str): version.
                lib_path (str): Library directory.
                futures (list): Future list.
            """
            include = os.path.join(lib_path, 'include')
            for lib_name in LIB_NAMES:
                futures.append(executor.submit(
                    dump_abi, str(tmpdir.join(
                        '%s_%s.abidump' % (lib_name, lib_version))),
                    os.path.join(lib_path, '%s.so' % lib_name), include,
                    lib_version, lib_name))

        # Get references versions
        abi_version = accelize_drm.get_api_version().major
        versions = executor.submit(get_reference_versions, tmpdir, abi_version)

        # Build reference versions
        versions = versions.result()
        if not versions:
            pytest.skip(
                'No previous versions with ABI version %s' % abi_version)

        print('CHECKING ABI/API AGAINST VERSIONS:', ', '.join(versions))
        for version, path in versions.items():
            build_futures.append(executor.submit(make_tag, version, path))

        # Waits for tools build completion
        for future in as_completed(tools_futures):
            future.result()

        # Dump latest version ABI and API
        dumps_library('latest', '.', latest_futures)

        # Dumps reference versions ABI and API
        for future in as_completed(build_futures):
            version, path = future.result()
            dumps_library(version, path, dump_futures)

        # Waits for latest version dump completion
        for future in as_completed(latest_futures):
            _, dump_file, name = future.result()
            latest_dumps[name] = dump_file

        # Compare latest ABI / API dumps with reference versions
        for future in as_completed(dump_futures):
            version, dump_file, name = future.result()

            reports[' '.join((name, version))] = executor.submit(
                checks_abi_compliance,  old_dump=dump_file,
                new_dump=latest_dumps[name], name=name,
                report_path=str(tmpdir.join('%s%s.html' % (name, version))))

    # Analyses reports
    abi_broken = False
    for title, future in reports.items():
        report = future.result()
        if ('Total binary compatibility problems: 0' not in report or
                'Total source compatibility problems: 0,' not in report):
            abi_broken = True
            print('Comparison against %s:\n%s\n' % (title, report))

    assert not abi_broken
