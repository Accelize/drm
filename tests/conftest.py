# -*- coding: utf-8 -*-
"""Configure Pytest"""
from os import environ, listdir, remove
from os.path import realpath, isfile, expanduser, splitext, join, dirname
from json import dump, load
from copy import deepcopy
from re import search

import pytest

_TESTS_PATH = dirname(realpath(__file__))
_SESSION = dict()
_LICENSING_SERVERS = dict(
    dev='https://master.devmetering.accelize.com',
    prod='https://master.metering.accelize.com')


def get_default_conf_json(licensing_server_url):
    """
    Get default "conf.json" file content as python dict.

    Args:
        licensing_server_url (str): Licensing server URL.

    Returns:
        dict: "conf.json" content
    """
    url = _LICENSING_SERVERS.get(
        licensing_server_url.lower(), licensing_server_url)

    return {
        "licensing": {
            "url": url,
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
            "oauth2_url": "%s/o/token/" % url,
            "metering_url":
                "%s/auth/metering/genlicense/" % url
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
        default="prod", help='Specify the metering server to use')
    parser.addoption(
        "--library_verbosity", action="store", default='4',
        help='Specify "libaccelize_drm" verbosity level')
    parser.addoption(
        "--library_logformat", action="store", default='0',
        help='Specify "libaccelize_drm" log format')
    parser.addoption(
        "--fpga_image", default="default",
        help='Select FPGA image to program the FPGA with. '
             'By default, use default FPGA image for the selected driver and '
             'last HDK version. Set to empty string to not program the FPGA.')
    parser.addoption(
        "--hdk_version",
        help='Select FPGA image base on Accelize DRM HDK version. By default, '
             'use default FPGA image for the selected driver and last HDK '
             'version.')
    parser.addoption(
        "--integration", action="store_true",
        help='Run integration tests. Theses tests may needs two FPGAs.'
    )


def pytest_runtest_setup(item):
    """
    Configure test initialization
    """
    markers = tuple(item.iter_markers(name='integration'))
    if not item.config.getoption("integration") and markers:
        pytest.skip("Don't run integration tests.")
    elif item.config.getoption("integration") and not markers:
        pytest.skip("Run only integration tests.")


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

    # Check cred.json
    if not isfile(realpath(expanduser(pytestconfig.getoption("cred")))):
        raise ValueError('Credential file specified by "--cred" do not exists')

    # Select C or C++ based on environment and import Python Accelize Library
    backend = pytestconfig.getoption("backend")
    if backend == 'c':
        environ['ACCELIZE_DRM_PYTHON_USE_C'] = '1'

    elif backend != 'c++':
        raise ValueError('Invalid value for "--backend"')

    import accelize_drm as _accelize_drm

    # Get FPGA driver
    from python_fpga_drivers import get_driver
    fpga_driver_name = pytestconfig.getoption("fpga_driver")
    fpga_driver_cls = get_driver(fpga_driver_name)

    # Get FPGA image
    fpga_image = pytestconfig.getoption("fpga_image")
    hdk_version = pytestconfig.getoption("hdk_version")

    if hdk_version and fpga_image.lower() != 'default':
        raise ValueError(
            'Please set "hdk_version" or "fpga_image" but not both')

    elif fpga_image.lower() == 'default' or hdk_version:
        # List available HDK versions for specified driver
        ref_designs = join(_TESTS_PATH, 'refdesigns', fpga_driver_name)
        hdk_versions = sorted([splitext(file_name)[0].strip('v')
                               for file_name in listdir(ref_designs)
                               if file_name.endswith('.json')])
        # Use specified HDK version
        if hdk_version:
            hdk_version = hdk_version.strip('v')
            if hdk_version not in hdk_versions:
                raise ValueError((
                    'HDK version %s is not supported. '
                    'Available versions are: %s') % (
                    hdk_version, ", ".join(hdk_versions)))

        # Get last HDK version as default
        else:
            hdk_version = hdk_versions[-1]

        # Get FPGA image from HDK version
        with open(join(ref_designs, 'v%s.json' % hdk_version)) as hdk_json_file:
            hdk_json = load(hdk_json_file)

        for key in ('fpga_image', 'FpgaImageGlobalId', 'FpgaImageId'):
            try:
                fpga_image = hdk_json[key]
                break
            except KeyError:
                continue
        else:
            raise ValueError('No FPGA image found for %s.' % hdk_version)

    # Define or get FPGA Slot
    if pytestconfig.getoption('integration'):
        # Integration tests requires 2 slots
        fpga_slot_id = [0, 1]
    elif environ.get('TOX_PARALLEL_ENV'):
        # Define FPGA slot for Tox parallel execution
        fpga_slot_id = [0 if backend == 'c' else 1]
    else:
        # Use user defined slot
        fpga_slot_id = [pytestconfig.getoption("fpga_slot_id")]

    # Initialize FPGA
    print('FPGA SLOT ID:', fpga_slot_id)
    print('FPGA IMAGE:', fpga_image)
    print('HDK VERSION:', hdk_version)
    fpga_driver = [fpga_driver_cls(
        fpga_slot_id=slot_id,
        fpga_image=fpga_image,
        drm_ctrl_base_addr=pytestconfig.getoption(
            "drm_controller_base_address"))
        for slot_id in fpga_slot_id]

    # Store some values for access in tests
    _accelize_drm.pytest_build_environment = build_environment
    _accelize_drm.pytest_build_source_dir = '@CMAKE_CURRENT_SOURCE_DIR@'
    _accelize_drm.pytest_build_type = build_type
    _accelize_drm.pytest_backend = backend
    _accelize_drm.pytest_fpga_driver = fpga_driver
    _accelize_drm.pytest_fpga_image = fpga_image
    _accelize_drm.pytest_hdk_version = hdk_version

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

    def __init__(self, tmpdir, url, **kwargs):
        content = get_default_conf_json(url)
        for k, v in kwargs.items():
            content[k] = v
        _Json.__init__(self, tmpdir, 'conf.json', content)


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

    def set_user(self, user=None):
        """
        Set user to use.

        Args:
            user (str): User to use. If not specified, use default user.
        """
        if user is None:
            self['client_id'] = self._initial_content['client_id']
            self['client_secret'] = self._initial_content['client_secret']
        else:
            try:
                self['client_id'] = self._initial_content['client_id_%s' % user]
                self['client_secret'] = self._initial_content['client_secret_%s' % user]
            except KeyError:
                raise ValueError( 'User "%s" not found in "%s"' % (
                        user, self._init_cref_path))
        self.save()


@pytest.fixture
def conf_json(pytestconfig, tmpdir):
    """
    Manage "conf.json" in testing environment.
    """
    log_param = { 'log_verbosity': int(pytestconfig.getoption("library_verbosity")),
                  'log_format': int(pytestconfig.getoption("library_logformat")) }
    json_conf = ConfJson(tmpdir, pytestconfig.getoption("server"), settings=log_param)
    json_conf.save()
    return json_conf


@pytest.fixture
def cred_json(pytestconfig, tmpdir):
    """
    Manage "cred.json" in testing environment.
    """
    return CredJson(
        tmpdir, realpath(expanduser(pytestconfig.getoption("cred"))))


def _get_session_info():
    """
    Get session information in case of Tox run.

    Returns:
        dict: Session information.
    """
    if _SESSION.get('current_session_name') is None:

        current_session_name = environ.get('TOX_ENV_NAME')
        if current_session_name:

            backend, build_type = current_session_name.split('-')
            other_session_name = '-'.join((
                'c' if backend == 'cpp' else 'c', build_type))

            _SESSION.update(dict(
                current_session_name=current_session_name,
                current_session_lock=current_session_name + '.lock',
                other_session_name=other_session_name,
                other_session_lock=other_session_name + '.lock'))
    return _SESSION


def pytest_sessionstart(session):
    """
    Pytest session initialization

    Args:
        session (pytest.Session): Current Pytest session.
    """
    # Get session information
    current_session_lock = _get_session_info().get('current_session_name')

    if current_session_lock is None:
        return

    # Create lock to indicate session is running
    with open(current_session_lock, 'w'):
        pass


def pytest_sessionfinish(session):
    """
    Pytest session ending

    Args:
        session (pytest.Session): Current Pytest session.
    """
    # Get session information
    session_info = _get_session_info()
    current_session_lock = session_info.get('current_session_name')
    if current_session_lock is None:
        return

    # Delete lock to indicate session is terminated
    remove(current_session_lock)

    # If other session is also terminated, remove all locks
    if not isfile(session_info.get('other_session_lock')):
        for file in listdir('.'):
            if splitext(file)[1] == '.lock':
                remove(file)


def perform_once(test_name):
    """
    Function that skip test if already performed in another session of a Tox
    parallel run.

    Useful for tests that do not depends on the Python library backend.

    Args:
        test_name (str): Test name.

    Returns:
        function: Patched test function.
    """
    test_lock = test_name + '.lock'

    # Skip test if lock exists.
    if isfile(test_lock):
        pytest.skip(
            'Test "%s" already performed in another session.' % test_name)

    # Create lock
    else:
        with open(test_lock, 'w'):
            pass


class AsyncErrorHandler:
    def __init__(self):
        self.reset()
    def reset(self):
        self.message = None
        self.errcode = None
        self.was_called = False
    def callback(self, message):
        self.was_called = True
        if isinstance(message, bytes):
            self.message = message.decode()
        else:
            self.message = message
        m = search(r'\[errCode=(\d+)\]', self.message)
        if m:
            self.errcode = int(m.group(1))
        else:
            self.errcode = None
    def assert_NoError( self, extra_msg=None ):
        if extra_msg is None:
            prepend_msg = ''
        else:
            prepend_msg = '%s: ' % extra_msg
        assert self.message is None, '%sAsynchronous callback reports a message: %s' % (prepend_msg, self.message)
        assert self.errcode is None, '%sAsynchronous callback returned error code: %d' % (prepend_msg, self.errcode)
        assert not self.was_called, '%sAsynchronous callback has been called' % prepend_msg


class AsyncErrorHandlerList(list):
    def create(self):
        cb = AsyncErrorHandler()
        super(AsyncErrorHandlerList, self).append(cb)
        return cb
    def parse_error_code(self, msg):
        from re import search
        match = search(r'\[errCode=(\d+)\]', msg)
        assert match, "Could not find 'errCode' in exception message: %s" % msg
        return int(match.group(1))


@pytest.fixture
def async_handler():
    return AsyncErrorHandlerList()

