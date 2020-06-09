# -*- coding: utf-8 -*-
"""Configure Pytest"""
from copy import deepcopy
from json import dump, load
from os import environ, getpid, listdir, remove, makedirs, getcwd, urandom
from os.path import basename, dirname, expanduser, isdir, isfile, join, \
    realpath, splitext
from random import randint
from re import IGNORECASE, match, search
from datetime import datetime
from time import time, sleep
from shutil import rmtree
from datetime import datetime, timedelta

import pytest

from tests.ws_admin_functions import WSListFunction


_SESSION = dict()
_LICENSING_SERVERS = dict(
    dev='https://master.devmetering.accelize.com',
    prod='https://master.metering.accelize.com')

ACT_STATUS_REG_OFFSET = 0x38
MAILBOX_REG_OFFSET = 0x3C
INC_EVENT_REG_OFFSET = 0x40


def bit_not(n, numbits=32):
    return (1 << numbits) - 1 - n


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
            "nodelocked": False
        },
        "drm": {
            "frequency_mhz": 125
        },
        "design": {
            "boardType": "Running on AWS"
        },
    }


def clean_nodelock_env(drm_manager=None, driver=None,
                       conf_json=None, cred_json=None, ws_admin=None,  product_name=None):
    """
    Clean nodelock related residues
    """
    # Clean license directory for nodelock
    if conf_json is not None:
        conf_json.cleanNodelockDir()
    # Clear nodelock request from WS DB (not to hit the limit)
    if product_name is None:
        product_name = 'drm_1activator'
    if (ws_admin is not None) and (cred_json is not None):
        ws_admin.remove_product_information(library='refdesign', name=product_name,
                                            user=cred_json.email)
    # Reprogram FPGA
    if driver is not None:
        if drm_manager is None:
            driver.program_fpga()
        elif drm_manager.get('drm_license_type') == 'Node-Locked':
            driver.program_fpga()


def clean_metering_env(cred_json=None, ws_admin=None, product_name=None):
    """
    Clean floating related residues
    """
    if product_name is None:
        product_name = 'drm_1activator'
    # Clear metering request from WS DB (not to hit the limit)
    if (ws_admin is not None) and (cred_json is not None):
        ws_admin.remove_product_information(library='refdesign',
            name=product_name, user=cred_json.email)


def param2dict(param_list):
    if param_list is None:
        return None
    d = dict()
    for e in param_list.split(','):
        k,v = e.split('=')
        try:
            d[k] = int(v)
        except ValueError:
            pass
        else:
            continue
        try:
            d[k] = float(v)
        except ValueError:
            pass
        else:
            continue
        d[k] = str(v)
    return d


# Pytest configuration
def pytest_addoption(parser):
    """
    Add command lines arguments
    """
    parser.addoption(
        "--backend", action="store", default="c++",
        help='Use specified Accelize DRM library API as backend: "c" or "c++"')
    parser.addoption(
        "--fpga_driver", action="store", default=None,
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
        "--library_verbosity", action="store", type=int, choices=list(range(7)),
        default=2, help='Specify "libaccelize_drm" verbosity level')
    parser.addoption(
        "--library_format", action="store", type=int, choices=(0, 1), default=0,
        help='Specify "libaccelize_drm" logging format: 0=short, 1=long')
    parser.addoption(
        "--no_clear_fpga", action="store_true", help='Bypass clearing of FPGA at start-up')
    parser.addoption(
        "--logfile", action="store_true", help='Save log to file')
    parser.addoption(
        "--proxy_debug", action="store_true", default=False,
        help='Activate debug for proxy')
    parser.addoption(
        "--fpga_image", default="default",
        help='Select FPGA image to program the FPGA with. '
             'By default, use default FPGA image for the selected driver and '
             'last HDK version.')
    parser.addoption(
        "--hdk_version", default=None,
        help='Select FPGA image base on Accelize DRM HDK version. By default, '
             'use default FPGA image for the selected driver and last HDK '
             'version.')
    parser.addoption(
        "--integration", action="store_true",
        help='Run integration tests. Theses tests may needs two FPGAs.')
    parser.addoption(
        "--activator_base_address", action="store", default=0x10000, type=int,
        help=('Specify the base address of the 1st activator. '
              'The other activators shall be separated by an address gap of '
              '0x10000'))
    parser.addoption(
        "--params", action="store", default=None,
        help='Specify a list of key=value pairs separated by a coma used '
             'for one or multiple tests: '
             '"--params key1=value1,key2=value2,..."')
    parser.addoption(
        "--artifacts_dir", action="store", default=getcwd(),
        help='Specify pytest artifacts directory')


def pytest_runtest_setup(item):
    """
    Configure test initialization
    """
    # Check integration tests
    markers = tuple(item.iter_markers(name='no_parallel'))
    markers += tuple(item.iter_markers(name='on_2_fpga'))
    if not item.config.getoption("integration") and markers:
        pytest.skip("Don't run integration tests.")
    elif item.config.getoption("integration") and not markers:
        pytest.skip("Run only integration tests.")

    # Check endurance tests
    m_option = item.config.getoption('-m')
    if search(r'\bendurance\b', m_option) and not search(r'\nnot\n\s+\bendurance\b', m_option):
        skip_endurance = False
    else:
        skip_endurance = True
    markers = tuple(item.iter_markers(name='endurance'))
    if markers and skip_endurance:
        pytest.skip("Don't run endurance tests.")

    # Check AWS execution
    markers = tuple(item.iter_markers(name='aws'))
    if '${AWS}' == 'OFF' and markers:
        pytest.skip("Don't run C/C++ function tests.")
    # Skip 'security' test if not explicitly marked
    for marker in item.iter_markers():
        if 'security' == marker.name and 'security' not in item.config.option.markexpr:
            pytest.skip('"security" marker not selected')


class SingleActivator:
    """
    SingleActivator object
    """
    def __init__(self, driver, base_address):
        self.driver = driver
        self.base_address = base_address
        self.metering_data = 0

    def autotest(self, is_activated=None):
        """
        Verify IP works as expected
        """
        # Test IP mailbox depending on activation status
        activated = self.get_status()
        if is_activated is not None:
            assert activated == is_activated
        else:
            if activated:
                # If unlocked writing the mailbox should succeed
                val = self.driver.read_register(self.base_address + MAILBOX_REG_OFFSET)
                not_val = bit_not(val)
                self.driver.write_register(self.base_address + MAILBOX_REG_OFFSET, not_val)
                assert self.driver.read_register(self.base_address + MAILBOX_REG_OFFSET) == not_val
            else:
                # If locked writing the mailbox should fail
                val = self.driver.read_register(self.base_address + MAILBOX_REG_OFFSET)
                not_val = bit_not(val)
                self.driver.write_register(self.base_address + MAILBOX_REG_OFFSET, not_val)
                assert self.driver.read_register(self.base_address + MAILBOX_REG_OFFSET) == val
            # Test reading of the generate event register
            assert self.driver.read_register(self.base_address + INC_EVENT_REG_OFFSET) == 0x600DC0DE
            # Test address overflow
            assert self.driver.read_register(self.base_address + INC_EVENT_REG_OFFSET + 0x4) == 0xDEADDEAD

    def get_status(self):
        """
        Get activation status.

        Returns:
            int: Status.
        """
        return self.driver.read_register(self.base_address+ACT_STATUS_REG_OFFSET) == 3

    def generate_coin(self, coins):
        """
        Generate coins.

        Args:
            coins (int): Number of coins to generate.
        """
        for _ in range(coins):
            self.driver.write_register(self.base_address+INC_EVENT_REG_OFFSET, 0)
        if self.get_status():
            self.metering_data += coins

    def reset_coin(self):
        """
        Reset the coins counter
        """
        self.metering_data = 0

    def check_coin(self, coins):
        """
        Compare coins to the expected value.

        Args:
            coins (int): Number of coins to compare to.
        """
        assert self.metering_data == coins


class ActivatorsInFPGA:
    """
    Activators object
    """
    def __init__(self, driver, base_address_list):
        self.activators = list()
        for addr in base_address_list:
            self.activators.append(SingleActivator(driver, addr))
        self.product_id = {
            "vendor": "accelize.com",
            "library": "refdesign",
            "name": "drm_%dactivator" % len(base_address_list),
            "sign": ""
        }

    def __getitem__(self, index):
        return self.activators[index]

    @property
    def length(self):
        """
        Number of activators in design/FPGA

        Returns:
            int: Number of activators
        """
        return len(self.activators)

    def autotest(self, is_activated=None, index=None):
        if index is None:
            index_list = list(range(len(self.activators)))
        else:
            index_list = [index]
        for i in index_list:
            self.activators[i].autotest(is_activated)


    def get_status(self, index=None):
        if index is None:
            index_list = list(range(len(self.activators)))
        else:
            index_list = [index]
        status_list = []
        for i in index_list:
            status_list.append(self.activators[i].get_status())
        if index is None:
            return status_list
        return status_list[0]

    def is_activated(self, index=None):
        return all(self.get_status(index))

    def reset_coin(self):
        """
        Reset the coins counter
        """
        for activator in self.activators:
            activator.reset_coin()




class RefDesign:
    """
    Handle HDK versions and their related FPGA image ID
    """
    def __init__(self, path):
        if not isdir(path):
            raise IOError("Following path must be a valid directory: %s" % path)
        self._path = path
        self.image_files = {splitext(file_name)[0].strip('v'):realpath(join(self._path, file_name))
                                    for file_name in listdir(self._path)}
        self.hdk_versions = sorted(filter(lambda x: match(r'^\d+', x), self.image_files.keys()))

    def get_image_id(self, hdk_version=None):
        if hdk_version is None:
            hdk_version = self.hdk_versions[-1]
        elif hdk_version not in self.image_files.keys():
            return None
        filename = join(self._path, self.image_files[hdk_version])
        ext = splitext(filename)[1]
        try:
            if ext == '.json':
                with open(filename, 'rt') as fp:
                    return load(fp)['FpgaImageGlobalId']
            elif ext == '.awsxclbin':
                return self.image_files[hdk_version]
                with open(filename, 'rb') as fp:
                    return search(r'(agfi-[0-9a-fA-F]+)', str(fp.read())).group(1)
        except Exception as e:
            raise Exception('No FPGA image found for %s: %s' % (hdk_version, str(e)))


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
        # NOTE: It is not possible to setup "LD_LIBRARY_PATH" once Python is running
        #       this value need to be set prior to run "pytest" to allow
        #       python importing shared libraries.
        import sys
        sys.path.append(realpath('python3_bdist'))

    else:
        build_environment = False
        build_type = 'release'

    # Check cred.json
    cred_path = realpath(expanduser(pytestconfig.getoption("cred")))
    if not isfile(cred_path):
        raise ValueError('Credential file specified by "--cred" does not exist')

    # Select C or C++ based on environment and import Python Accelize Library
    backend = pytestconfig.getoption("backend")
    if backend == 'c':
        environ['ACCELIZE_DRM_PYTHON_USE_C'] = '1'

    elif backend != 'c++':
        raise ValueError('Invalid value for "--backend"')

    # Get FPGA image
    fpga_image = pytestconfig.getoption("fpga_image")
    hdk_version = pytestconfig.getoption("hdk_version")
    if hdk_version and fpga_image.lower() != 'default':
        raise ValueError(
            'Mutually exclusive options: Please set "hdk_version" or "fpga_image", but not both')

    # Get FPGA driver
    fpga_driver_name = pytestconfig.getoption("fpga_driver")
    if fpga_driver_name and fpga_image.lower() != 'default':
        raise ValueError(
            'Mutually exclusive options: Please set "fpga_driver" or "fpga_image", but not both')
    if fpga_image.lower() != 'default':
        if fpga_image.endswith('.awsxclbin'):
            fpga_driver_name = 'xilinx_xrt'
        elif search(r'agfi-[0-9a-f]+', fpga_image, IGNORECASE):
            fpga_driver_name = 'aws_f1'
        else:
            raise ValueError("Unsupported 'fpga_image' option")
    elif fpga_driver_name is None:
        fpga_driver_name = 'aws_f1'

    # Get cmake building directory
    build_source_dir = '@CMAKE_CURRENT_SOURCE_DIR@'
    if build_source_dir.startswith('@'):
        build_source_dir = realpath('.')

    # Create pytest artifacts directory
    pytest_artifacts_dir = join(pytestconfig.getoption("artifacts_dir"), 'pytest_artifacts')
    if not isdir(pytest_artifacts_dir):
        makedirs(pytest_artifacts_dir)
    print('pytest artifacts directory: ', pytest_artifacts_dir)

    # Get Ref Designs available
    ref_designs = RefDesign(join(build_source_dir, 'tests', 'refdesigns', fpga_driver_name))

    if fpga_image.lower() == 'default' or hdk_version:
        # Use specified HDK version
        if hdk_version:
            hdk_version = hdk_version.strip('v')
            if hdk_version not in ref_designs.hdk_versions:
                raise ValueError((
                    'HDK version %s is not supported. '
                    'Available versions are: %s') % (
                    hdk_version, ", ".join(ref_designs.hdk_versions)))
        # Get last HDK version as default
        else:
            hdk_version = ref_designs.hdk_versions[-1]
        # Get FPGA image from HDK version
        fpga_image = ref_designs.get_image_id(hdk_version)

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

    # Get FPGA driver
    from tests.fpga_drivers import get_driver
    fpga_driver_cls = get_driver(fpga_driver_name)

    # Initialize FPGA
    no_clear_fpga = pytestconfig.getoption("no_clear_fpga")
    drm_ctrl_base_addr = pytestconfig.getoption("drm_controller_base_address")
    print('FPGA SLOT ID:', fpga_slot_id)
    print('FPGA IMAGE:', basename(fpga_image))
    print('HDK VERSION:', hdk_version)
    fpga_driver = list()
    for slot_id in fpga_slot_id:
        try:
            fpga_driver.append(
                fpga_driver_cls( fpga_slot_id=slot_id,
                    fpga_image=fpga_image,
                    drm_ctrl_base_addr=drm_ctrl_base_addr,
                    no_clear_fpga=no_clear_fpga
                )
            )
        except:
            raise IOError("Failed to load driver on slot %d" % slot_id)

    # Define Activator access per slot
    fpga_activators = list()
    for driver in fpga_driver:
        base_addr_list = []
        base_address = pytestconfig.getoption("activator_base_address")
        while True:
            val = driver.read_register(base_address + INC_EVENT_REG_OFFSET)
            if val != 0x600DC0DE:
                break
            base_addr_list.append(base_address)
            base_address += 0x10000
        fpga_activators.append(ActivatorsInFPGA(driver, base_addr_list))
        if len(base_addr_list) == 0:
            raise IOError('No activator found on slot #%d' % driver._fpga_slot_id)
        print('Found %d activator(s) on slot #%d' % (len(base_addr_list), driver._fpga_slot_id))

    # Store some values for access in tests
    import accelize_drm as _accelize_drm
    _accelize_drm.pytest_new_freq_method_supported = fpga_driver[0].read_register(drm_ctrl_base_addr + 0xFFF8) == 0x60DC0DE0
    _accelize_drm.pytest_proxy_debug = pytestconfig.getoption("proxy_debug")
    _accelize_drm.pytest_server = pytestconfig.getoption("server")
    _accelize_drm.pytest_build_environment = build_environment
    _accelize_drm.pytest_build_source_dir = build_source_dir
    _accelize_drm.pytest_build_type = build_type
    _accelize_drm.pytest_backend = backend
    _accelize_drm.pytest_fpga_driver = fpga_driver
    _accelize_drm.pytest_fpga_driver_name = fpga_driver_name
    _accelize_drm.pytest_fpga_slot_id = fpga_slot_id
    _accelize_drm.pytest_fpga_image = fpga_image
    _accelize_drm.pytest_hdk_version = hdk_version
    _accelize_drm.pytest_fpga_activators = fpga_activators
    _accelize_drm.pytest_ref_designs = ref_designs
    _accelize_drm.clean_nodelock_env = lambda *kargs, **kwargs: clean_nodelock_env(
        *kargs, **kwargs, product_name=fpga_activators[0].product_id['name'])
    _accelize_drm.clean_metering_env = lambda *kargs, **kwargs: clean_metering_env(
        *kargs, **kwargs, product_name=fpga_activators[0].product_id['name'])
    _accelize_drm.pytest_params = param2dict(pytestconfig.getoption("params"))
    _accelize_drm.pytest_artifacts_dir = pytest_artifacts_dir

    return _accelize_drm


class _Json:
    """Json file"""

    def __init__(self, tmpdir, name, content):
        self._dirname = str(tmpdir)
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

    def addNodelock(self):
        self['licensing']['nodelocked'] = True
        self['licensing']['license_dir'] = dirname(self._path)
        assert isdir(self['licensing']['license_dir'])
        self.save()

    def removeNodelock(self):
        if 'nodelocked' in self['licensing']:
            del self['licensing']['nodelocked']
        assert 'nodelocked' not in self['licensing']
        if 'license_dir' in self['licensing'].keys():
            del self['licensing']['license_dir']
        assert 'license_dir' not in self['licensing']
        self.save()

    def cleanNodelockDir(self):
        from glob import glob
        file_list = glob(join(self._dirname, '*.req'))
        file_list.extend(glob(join(self._dirname, '*.lic')))
        for e in file_list:
            remove(e)


class CredJson(_Json):
    """cred.json file"""

    def __init__(self, tmpdir, path):
        self._init_cred_path = path
        try:
            with open(path, 'rt') as cref_file:
                cred = load(cref_file)
        except OSError:
            cred = dict(client_id='', secret_id='')
        # Load from user specified cred.json
        _Json.__init__(self, tmpdir, 'cred.json', cred)
        self._user = ''

    def set_user(self, user=None):
        """
        Set user to use.

        Args:
            user (str): User to use. If not specified, use default user.
        """
        self._content = {}
        if user is None:
            for k, v in [e for e in self._initial_content.items() if not e.endswith('__')]:
                self[k] = v
            self._user = ''
        else:
            for k, v in self._initial_content.items():
                m = match(r'(.+)__%s__' % user, k)
                if m:
                    self[m.group(1)] = v
            self._user = user
        if ('client_id' not in self._content) or ('client_secret' not in self._content):
            raise ValueError('User "%s" not found in "%s"' % (user, self._init_cred_path))
        self.save()

    def get_user(self, user=None):
        """
        Return user details.

        Args:
            user (str): User to get. If not specified, use default user.
        """
        content = {}
        if user is None:
            for k, v in [e for e in self._initial_content.items() if not e.endswith('__')]:
                content[k] = v
            content['user'] = ''
        else:
            for k, v in self._initial_content.items():
                m = match(r'(.+)__%s__' % user, k)
                if m:
                    content[m.group(1)] = v
            content['user'] = user
        if ('client_id' not in content) or ('client_secret' not in content):
            raise ValueError('User "%s" not found in "%s"' % (user, self._init_cred_path))
        return content

    @property
    def user(self):
        return self._user

    @property
    def email(self):
        return self['email']

    @property
    def client_id(self):
        return self['client_id']

    @client_id.setter
    def client_id(self, s):
        self['client_id'] = s

    @property
    def client_secret(self):
        return self['client_secret']

    @client_secret.setter
    def client_secret(self, s):
        self['client_secret'] = s


@pytest.fixture
def conf_json(pytestconfig, tmpdir):
    """
    Manage "conf.json" in testing environment.
    """
    log_param = {'log_verbosity': pytestconfig.getoption("library_verbosity")}
    if pytestconfig.getoption("library_format") == 1:
        log_param['log_format'] = '%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v'
    else:
        log_param['log_format'] = '[%^%=8l%$] %-6t, %v'
    if pytestconfig.getoption("logfile"):
        log_param['log_file_type'] = 1
        log_param['log_file_path'] = realpath("./drmlib_t%f_pid%d.log" % (time(), getpid()))
        log_param['log_file_verbosity'] = pytestconfig.getoption("library_verbosity")
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


def wait_func_true(func, timeout=None, sleep_time=1):
    """
    Wait until the func retursn a none 0 value

    Args:
        func (__call__) : Function that is run and evaluated
        timeout (int) : Specify a timeout in seconds for the loop
        sleep_time (int) : Sleep in seconds befaore reexecuting the function

    Returns:
        boolean: True if the output of the function has been evaluated to True, False otherwise
    """
    start = datetime.now()
    while not func():
        if timeout:
            if (datetime.now() - start) > timedelta(seconds=sleep_time):
                return False
        sleep(sleep_time)
    return True


def wait_deadline(start_time, duration):
    """
    Wait until endtime is hit

    Args:
        start_time (datetime): start time of the timer.
        duration to wait for (int): duration in seconds to wait for.
    """
    wait_period = start_time + timedelta(seconds=duration) - datetime.now()
    if wait_period.total_seconds() <= 0:
        return
    sleep(wait_period.total_seconds())



class AsyncErrorHandler:
    """
    Asynchronous error callback
    """
    def __init__(self):
        self.message = None
        self.errcode = None
        self.was_called = False

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
        self.errcode = AsyncErrorHandlerList.get_error_code(self.message)

    def assert_NoError(self, extra_msg=None):
        if extra_msg is None:
            prepend_msg = ''
        else:
            prepend_msg = '%s: ' % extra_msg
        assert self.message is None, '%sAsynchronous callback reports a message: %s' \
                                     % (prepend_msg, self.message)
        assert self.errcode is None, '%sAsynchronous callback returned error code: %d' \
                                     % (prepend_msg, self.errcode)
        assert not self.was_called, '%sAsynchronous callback has been called' % prepend_msg


class AsyncErrorHandlerList(list):
    """
    Handle a list of asynchronous error callbacks and error message parsing
    """
    def create(self):
        cb = AsyncErrorHandler()
        super(AsyncErrorHandlerList, self).append(cb)
        return cb

    @staticmethod
    def get_error_code(msg):
        from re import search
        m = search(r'\[errCode=(\d+)\]', msg)
        assert m, "Could not find 'errCode' in exception message: %s" % msg
        return int(m.group(1))


@pytest.fixture
def async_handler():
    return AsyncErrorHandlerList()


class WSAdmin:
    """
    Handle Web Service administration for test and debug of the DRM Lib
    """
    def __init__(self, url, client_id, client_secret):
        self._functions = WSListFunction(url, client_id, client_secret)

    def remove_product_information(self, library, name, user):
        self._functions._get_user_token()
        data = {'library': library, 'name': name, 'user': user}
        text, status = self._functions.remove_product_information(data)
        assert status == 200, text

    def get_last_metering_information(self, session_id):
        self._functions._get_user_token()
        text, status = self._functions.metering_lastinformation({'session': session_id})
        assert status == 200, text
        return text

    @property
    def functions(self):
        return self._functions


@pytest.fixture
def ws_admin(cred_json, conf_json):
    cred = cred_json.get_user('admin')
    assert cred['user'] == 'admin'
    return WSAdmin(conf_json['licensing']['url'], cred['client_id'],
                   cred['client_secret'])


class ExecFunction:
    """
    Provide test functions using directly C or C++ object
    """
    def __init__(self, slot_id, is_cpp, test_file_name, conf_path, cred_path):
        self._conf_path = conf_path
        self._cred_path = cred_path
        self._is_cpp = is_cpp
        self._slot_id = slot_id
        self._test_func_path = join('@CMAKE_BINARY_DIR@', 'tests', test_file_name)
        if not isfile(self._test_func_path):
            raise IOError("No executable '%s' found" % self._test_func_path)
        self._cmd_line = '%s -s %d -f %s -d %s' % (self._test_func_path, self._slot_id,
                                                   self._conf_path, self._cred_path)
        if not self._is_cpp:
            self._cmd_line += ' -c'
        self.returncode = None
        self.stdout = None
        self.stderr = None
        self.asyncmsg = None

    def run(self, test_name=None):
        from subprocess import run, PIPE
        cmdline = self._cmd_line
        if test_name is not None:
            cmdline += ' -t %s' % test_name
        print('cmdline=', cmdline)
        result = run(cmdline, shell=True, stdout=PIPE, stderr=PIPE)
        self.returncode = result.returncode
        self.stdout = result.stdout.decode()
        self.stderr = result.stderr.decode()
        if self.stdout:
            print(self.stdout)
        if self.stderr:
            print(self.stderr)
        # Looking for asynchronous error
        m = search(r'\bAsyncErrorMessage=(.*)', self.stderr)
        if m:
            self.asyncmsg = m.group(1)


class ExecFunctionFactory:
    """
    Provide an object to load executable with test functions in C/C++
    """
    def __init__(self, conf_path, cred_path, is_cpp, is_release_build=False):
        self._conf_path = conf_path
        self._cred_path = cred_path
        self._is_cpp = is_cpp
        self._is_release_build = is_release_build

    def load(self, test_file_name, slot_id):
        try:
            return ExecFunction(slot_id, self._is_cpp, test_file_name, self._conf_path,
                                self._cred_path)
        except IOError:
            if self._is_release_build:
                pytest.skip("No executable '%s' found: test skipped" % self._test_func_path)


@pytest.fixture
def exec_func(accelize_drm, cred_json, conf_json):
    is_cpp = accelize_drm.pytest_backend == 'c++'
    is_release_build = 'release' in accelize_drm.pytest_build_type
    return ExecFunctionFactory(conf_json.path, cred_json.path, is_cpp, is_release_build)


#--------------
# Proxy fixture
#--------------

class EndpointAction:

    def __init__(self, action):
        self.action = action

    def __call__(self, *kargs, **kwargs):
        return self.action(*kargs, **kwargs)
        if isinstance(resp,Response):
            return resp
        return Response(msg, status=status, headers={})


class FlaskAppWrapper:

    def __init__(self, name=__name__, debug=False):
        from flask import Flask, session
        self.app = Flask(name)
        self.app.config['SECRET_KEY'] = 'super secret'
        self.debug = debug
        environ['WERKZEUG_RUN_MAIN'] = 'true'
        environ['FLASK_ENV'] = 'development'

    def run(self, host='127.0.0.1', port=5000):
        self.host = host
        self.port = port
        self.app.run(host=self.host, port=self.port, debug=self.debug)

    def add_endpoint(self, rule=None, endpoint=None, handler=None, **kwargs):
        self.app.add_url_rule(rule, endpoint, EndpointAction(handler), **kwargs)


@pytest.fixture
def fake_server(accelize_drm):
    name = "fake_server_%d" % randint(1,0xFFFFFFFF)
    return FlaskAppWrapper(name, accelize_drm.pytest_proxy_debug)


#-------------------
# Artifacts fixture
#-------------------

class ArtifactFactory:

    def __init__(self, artifacts_dir=None):
        self.artifact_dir = artifacts_dir
        # Create artifacts directory if not existing
        if not isdir(self.artifact_dir):
            makedirs(self.artifact_dir)

    def save_path(self, path, rename=None):
        from shutil import copy
        if rename:
            dst = join(self.artifact_dir, rename)
        else:
            dst = self.artifact_dir
        if isfile(path):
            copy(path, dst)
        elif isdir(path):
            copytree(path, dst)

    def save_content(self, content, filename, mode='wt'):
        file_path = join(self.artifact_dir, filename)
        with open(file_path, mode) as fw:
            fw.write(content)

@pytest.fixture
def artifacts(accelize_drm):
    return ArtifactFactory(accelize_drm.pytest_artifacts_dir)
