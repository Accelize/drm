# -*- coding: utf-8 -*-
"""
Test ABI/API compatibility
"""
from concurrent.futures import ThreadPoolExecutor, as_completed
import os
from subprocess import run, CalledProcessError, PIPE
import pytest

LIB_NAMES = ('libaccelize_drmc', 'libaccelize_drm')
REPOSITORY_PATH = 'https://github.com/Accelize/drmlib'


def _run(command):
    """
    Run a command.

    Args:
        command (str):

    Returns:
        subprocess.CompletedProcess
    """
    result = run(command, shell=True, stdout=PIPE, stderr=PIPE,
                 universal_newlines=True)
    try:
        result.check_returncode()
    except CalledProcessError as exception:
        print(exception.stdout, exception.stderr, sep='\n')
        raise
    return result


def make(path, target=None):
    """
    Make.

    Args:
        path (str): Path of sources to make.
        target (str): Target
    """
    _run(f'cd {path} && make -j {os.cpu_count()}'
         f'{(" " + target) if target else ""}')


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
    _run(f"abi-dumper {so_file} "
         f"-public-headers {include} -o {dump_file} -lver {version}")
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
    return _run(f"abi-compliance-checker -l {name} -old {old_dump} -new "
                f"{new_dump} -report-path {report_path}").stdout


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
    _run(f'git clone -q -b v{version} --depth 1 {REPOSITORY_PATH} {path}/src')

    # Make
    _run(f'cd {path} && cmake -DCMAKE_BUILD_TYPE=Debug ./src')
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
    tags = _run(f'git ls-remote --tags {REPOSITORY_PATH}').stdout
    versions = {
        tag: str(tmpdir.join(tag)) for tag in (
        tag.rsplit('/', 1)[1].strip('v/n') for tag in tags.splitlines())
        if (tag.split('.', 1)[0] == abi_version and
            tag.rsplit('.', 1)[1] == '0')}

    return versions


def test_abi_compliance(tmpdir, accelize_drm):
    """
    Test the ABI/API compliance of the lib_name.
    """
    if not accelize_drm.pytest_build_environment:
        pytest.skip('This test is only performed on build environment.')
    elif not accelize_drm.pytest_build_type == 'debug':
        pytest.xfail('This test need libraries compiled in debug mode.')

    # Initialize test
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
                    dump_abi,str(tmpdir.join(
                        f'{lib_name}_{lib_version}.abidump')),
                    os.path.join(lib_path, f'{lib_name}.so'), include,
                    lib_version, lib_name))

        # Get references versions
        abi_version = accelize_drm.get_api_version().major
        versions = executor.submit(get_reference_versions, tmpdir, abi_version)

        # Build reference versions
        versions = versions.result()
        if not versions:
            pytest.skip(
                f'No previous versions with ABI version {abi_version}')

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

            reports[f'{name} {version}'] = executor.submit(
                checks_abi_compliance,  old_dump=dump_file,
                new_dump=latest_dumps[name], name=name,
                report_path=str(tmpdir.join(f'{name}{version}.html')))

    # Analyses reports
    abi_broken = False
    for title, future in reports.items():
        report = future.result()
        if ('Total binary compatibility problems: 0' not in report or
                'Total source compatibility problems: 0,' not in report):
            abi_broken = True
            print(f'Comparison against {title}:\n{report}\n')

    assert not abi_broken
