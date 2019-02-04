# -*- coding: utf-8 -*-
"""Configure Pytest"""
from os import environ
from os.path import realpath, isfile, expanduser
from json import dump, load
import pytest


def pytest_addoption(parser):
    """
    Add command lines arguments
    """
    parser.addoption(
        "--backend", action="store", default="c++",
        help='Use specified Accelize DRM library API as backend: "c" or "c++"')
    parser.addoption(
        "--fpga_driver", action="store", default="aws_f1",
        help='Specify FPGA driver to use with DRM library')
    parser.addoption(
        "--fpga_slot_id", action="store", default=0, type=int,
        help='Specify the FPGA slot to use')
    parser.addoption(
        "--drm_controller_base_address", action="store", default=0, type=int,
        help='Specify the DRM controller base address')
    parser.addoption(
        "--cred", action="store", default="./cred.json",
        help='Specify cred.json path')
    parser.addoption(
        "--server", action="store",
        default="https://master.metering.accelize.com",
        help='Specify the metering server to use')
    parser.addoption(
        "--library_verbosity", action="store", default='4',
        help='Specify "libaccelize_drm" verbosity level')


@pytest.fixture(scope='session')
def accelize_drm(pytestconfig):
    """
    Get Python Accelize DRM configured the proper way.
    """
    # Define if currently in build environment
    if isfile('CMakeCache.txt'):
        build_environment = True
        with open('CMakeCache.txt', 'rt') as cmake_cache:
            build_type = (
                'debug' if "CMAKE_BUILD_TYPE:STRING=Debug" in cmake_cache.read()
                else 'release')

        # Add Build environment to Python import hook
        # NOTE: It is not possible to setup "LD_LIBRARY_PATH" once Python is run
        #       this value need to be set prior to run "pytest" to allow
        #       python importing shared libraries.
        import sys
        sys.path.append(realpath('python3_bdist'))

    else:
        build_environment = False
        build_type = 'release'

    # Set verbosity level
    verbosity = pytestconfig.getoption("library_verbosity")
    environ['ACCELIZE_DRM_VERBOSE'] = verbosity

    # Check cred.json
    if not isfile(realpath(expanduser(pytestconfig.getoption("cred")))):
        raise ValueError('Credential file specified by "--cred" do not exists')

    # Select C or C++ based on environment and import Python Accelize Library
    backend = pytestconfig.getoption("backend")
    if backend == 'c':
        # Import
        environ['ACCELIZE_DRM_PYTHON_USE_C'] = '1'
        import accelize_drm as _accelize_drm

        # Check imported class
        from accelize_drm._accelize_drmc import DrmManager
        assert _accelize_drm.DrmManager is DrmManager

    elif backend == 'c++':
        # Import
        import accelize_drm as _accelize_drm

        # Check imported class
        from accelize_drm._accelize_drm import DrmManager
        assert _accelize_drm.DrmManager is DrmManager

    else:
        raise ValueError('Invalid value for "--backend"')

    # Get FPGA driver
    from tests.fpga_drivers import get_driver
    fpga_driver_cls = get_driver(pytestconfig.getoption("fpga_driver"))
    fpga_driver_cls.DRM_CONTROLLER_BASE_ADDR = pytestconfig.getoption(
        "drm_controller_base_address")

    if environ.get('TOX_PARALLEL_ENV'):
        # Define FPGA slot for Tox parallel execution
        fpga_driver_cls.SLOT_ID = 0 if backend == 'c' else 1
    else:
        # Use user defined slot
        fpga_driver_cls.SLOT_ID = pytestconfig.getoption("fpga_slot_id")

    # Initialize FPGA
    fpga_driver = fpga_driver_cls()

    # Store some values for access in tests
    _accelize_drm.pytest_build_environment = build_environment
    _accelize_drm.pytest_build_type = build_type
    _accelize_drm.pytest_backend = backend
    _accelize_drm.pytest_fpga_driver = fpga_driver
    _accelize_drm.pytest_lib_verbosity = verbosity

    return _accelize_drm


class _Json:
    """Json file"""

    def __init__(self, tmpdir, name, content):
        self._path = str(tmpdir.join(name))
        self._content = content

    def __setitem__(self, key, value):
        self._content[key] = value

    def __getitem__(self, key):
        return self._content[key]

    @property
    def path(self):
        """
        File path

        Returns:
            str: path
        """
        return self._path

    def save(self):
        """
        Save configuration in file.
        """
        with open(self._path, 'wt') as json_file:
            dump(self._content, json_file)


class ConfJson(_Json):
    """conf.json file"""

    def __init__(self, tmpdir, url):
        _Json.__init__(self, tmpdir, 'conf.json', {
            "licensing": {
                "url": url,
            },

            # 1.X.X API compatibility
            # TODO: Remove once expired
            "design": {
                "udid": "6AE1A700-0000-0000-0000-000000000001",
                "boardType": "DRM_125"
            },
            "webservice": {
                "oauth2_url": f"{url}/o/token/",
                "metering_url": f"{url}/auth/metering/genlicense/",
                "minimum_license_duration": 60
            }
        })


class CredJson(_Json):
    """cred.json file"""

    def __init__(self, tmpdir, path):
        try:
            with open(path, 'rt') as cref_file:
                cred = load(cref_file)
        except OSError:
            cred = dict(client_id='', secret_id='')
        _Json.__init__(self, tmpdir, 'cred.json', cred)


@pytest.fixture
def conf_json(pytestconfig, tmpdir):
    """
    Manage "conf.json" in testing environment.
    """
    return ConfJson(tmpdir, pytestconfig.getoption("server"))


@pytest.fixture
def cred_json(pytestconfig, tmpdir):
    """
    Manage "cred.json" in testing environment.
    """
    return CredJson(
        tmpdir, realpath(expanduser(pytestconfig.getoption("cred"))))
