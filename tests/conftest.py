# -*- coding: utf-8 -*-
"""Configure Pytest"""
from os import environ
from os.path import realpath, isfile, expanduser
from json import dump, load
from copy import deepcopy
import pytest

# Default values
DEFAULT_FPGA_IMAGE = {
    'aws_f1': 'agfi-06938e283466fc386',
}


def get_default_conf_json(licensing_server_url):
    """
    Get default "conf.json" file content as python dict.

    Args:
        licensing_server_url (str): Licensing server URL.

    Returns:
        dict: "conf.json" content
    """
    return {
        "licensing": {
            "url": licensing_server_url,
        },
        "drm": {
            "frequency_mhz": 125
        },

        # 1.X.X API compatibility
        # TODO: Remove once expired
        "design": {
            "udid": "6AE1A700-0000-0000-0000-000000000001",
            "boardType": "DRM_125"
        },
        "webservice": {
            "oauth2_url": "%s/o/token/" % licensing_server_url,
            "metering_url":
                "%s/auth/metering/genlicense/" % licensing_server_url
        }
    }


# Pytest configuration

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
    parser.addoption(
        "--library_log_format", action="store", default='0',
        help='Specify "libaccelize_drm" log format')
    parser.addoption(
        "--fpga_image", default="default",
        help='Select FPGA image to use for program the FPGA. '
             'By default, use default FPGA image for the selected driver. '
             'Set to empty string to not program the FPGA.')


# Pytest Fixtures

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

    # Set log format
    log_format = pytestconfig.getoption("library_log_format")
    environ['ACCELIZE_DRM_LOG_FORMAT'] = log_format

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
    from python_fpga_drivers import get_driver
    fpga_driver_name = pytestconfig.getoption("fpga_driver")
    fpga_driver_cls = get_driver(fpga_driver_name)

    # Get FPGA image
    fpga_image = pytestconfig.getoption("fpga_image")
    if fpga_image.lower() == 'default':
        fpga_image = DEFAULT_FPGA_IMAGE.get(fpga_driver_name)

    # Define or get FPGA Slot
    if environ.get('TOX_PARALLEL_ENV'):
        # Define FPGA slot for Tox parallel execution
        fpga_slot_id = 0 if backend == 'c' else 1
    else:
        # Use user defined slot
        fpga_slot_id = pytestconfig.getoption("fpga_slot_id")

    # Initialize FPGA
    print('FPGA SLOT ID:', fpga_slot_id)
    print('FPGA IMAGE:', fpga_image)
    fpga_driver = fpga_driver_cls(
        fpga_slot_id=fpga_slot_id,
        fpga_image=fpga_image,
        drm_ctrl_base_addr=pytestconfig.getoption(
            "drm_controller_base_address"))

    # Store some values for access in tests
    _accelize_drm.pytest_build_environment = build_environment
    _accelize_drm.pytest_build_source_dir = '@CMAKE_CURRENT_SOURCE_DIR@'
    _accelize_drm.pytest_build_type = build_type
    _accelize_drm.pytest_backend = backend
    _accelize_drm.pytest_fpga_driver = fpga_driver
    _accelize_drm.pytest_fpga_image = fpga_image
    _accelize_drm.pytest_lib_verbosity = verbosity

    return _accelize_drm


class _Json:
    """Json file"""

    def __init__(self, tmpdir, name, content):
        self._path = str(tmpdir.join(name))
        self._content = content
        self._initial_content = deepcopy(content)
        self.save()

    def __delitem__(self, key):
        del self._content[key]

    def __setitem__(self, key, value):
        self._content[key] = value

    def __getitem__(self, key):
        return self._content[key]

    def __contains__(self, key):
        return key in self._content

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

    def reset(self):
        """
        Reset configuration to initial content.
        """
        self._content = deepcopy(self._initial_content)
        self.save()


class ConfJson(_Json):
    """conf.json file"""

    def __init__(self, tmpdir, url):
        _Json.__init__(self, tmpdir, 'conf.json', get_default_conf_json(url))


class CredJson(_Json):
    """cred.json file"""

    def __init__(self, tmpdir, path):
        self._init_cref_path = path
        try:
            with open(path, 'rt') as cref_file:
                cred = load(cref_file)
        except OSError:
            cred = dict(client_id='', secret_id='')

        # Load from user specified cred.json
        _Json.__init__(self, tmpdir, 'cred.json', cred)

        # Save current user as default
        self._default_client_id = self._content['client_id']
        self._default_client_secret = self._content['client_secret']

    def set_user(self, user=None):
        """
        Set user to use.

        Args:
            user (str): User to use. If not specified, use default user.
        """
        if user is None:
            self._content['client_id'] = self._default_client_id
            self._content['client_secret'] = self._default_client_secret
        else:
            try:
                self._content['client_id'] = self._content[
                    'client_id_%s' % user]
                self._content['client_secret'] = self._content[
                    'client_secret_' % user]
            except KeyError:
                raise ValueError(
                    'User "%s" not found in "%s"' % (
                        user, self._init_cref_path))
        self.save()


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
