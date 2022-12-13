# -*- coding: utf-8 -*-
"""
Check testing environment.
"""
import pytest
from tests.conftest import perform_once


@pytest.mark.hwtst
def test_versions_matches(accelize_drm):
    """
    Checks that the C/C++ library and the Python libraries versions matches.
    """
    api_version = accelize_drm.get_api_version()
    assert api_version.major == api_version.py_major
    assert api_version.minor == api_version.py_minor
    assert api_version.revision == api_version.py_revision
    assert api_version.prerelease == api_version.py_prerelease
    assert api_version.build == api_version.py_build


@pytest.mark.hwtst
def test_python_backend_library(accelize_drm, pytestconfig):
    """
    Checks that the Python library use the good C or C++ library as backend.
    """
    # Test: command line option passed
    assert accelize_drm.pytest_backend == pytestconfig.getoption("backend")

    # Test: imported backend
    backend = accelize_drm.get_api_version().backend
    if accelize_drm.pytest_backend == 'c':
        assert backend == 'libaccelize_drmc'
    else:
        assert backend == 'libaccelize_drm'


@pytest.mark.hwtst
def test_credentials(cred_json, conf_json):
    """
    Tests if credentials in "cred.json" are valid.
    """
    perform_once(__name__ + '.test_credentials')

    from http.client import HTTPSConnection
    from json import loads

    # Checks Client ID and secret ID presence
    client_id = cred_json['client_id']
    client_secret = cred_json['client_secret']
    assert client_id
    assert client_secret

    # Check OAuth credential validity
    def get_oauth_token():
        """Get OAuth token"""
        connection = HTTPSConnection(
            conf_json['licensing']['url'].split('//', 1)[1])
        connection.request(
            "POST", "/auth/token?client_id=%s&client_secret=%s&grant_type=client_credentials" % (
                client_id, client_secret))
        return connection.getresponse()

    for i in range(3):
        response = get_oauth_token()
        status = response.status
        if status == 401 or status < 300:
            break

    if not (200 <= response.status < 300):
        pytest.fail(response.read().decode())
    assert loads(response.read()).get('access_token')


@pytest.mark.hwtst
def test_fpga_drivers_base():
    """
    Test accelize_drm.fpga_drivers.FpgaDriverBase.
    """
    from tests.fpga_drivers import FpgaDriverBase
    from ctypes import c_uint32

    library = 'fpga_library.so'
    fpga_slot_id = 5
    base_address = 0x10
    fpga_image = 'fpga_image'

    class Fpga:
        """Fake FPGA"""
        fpga_image = None
        initialized = False
        register = {}

        @classmethod
        def read_register(cls, register_offset, returned_data):
            """Read register."""
            address = int(str(returned_data).rstrip('>)').split('(', 1)[1], 16)
            c_uint32.from_address(address).value = cls.register.get(
                register_offset)

        @classmethod
        def write_register(cls, register_offset, data_to_write):
            """Write register."""
            cls.register[register_offset] = data_to_write

    class FpgaDriver(FpgaDriverBase):
        """Fake FPGA driver"""

        def _get_driver(self):
            """Get FPGA driver"""
            return library

        def _get_lock(self):
            """Get FPGA driver lock"""
            return None

        def _clear_fpga(self):
            """Program the FPGA """
            return

        def _program_fpga(self, fpga_image):
            """Program the FPGA """
            Fpga.fpga_image = fpga_image

        def _init_fpga(self):
            """Initialize FPGA"""
            Fpga.initialized = True

        def _get_read_register_callback(self):
            """Read register callback."""
            return Fpga.read_register

        def _get_write_register_callback(self):
            """Write register callback."""
            return Fpga.write_register

    # Test instantiation without programming image
    assert not Fpga.initialized

    driver = FpgaDriver(
        fpga_slot_id=fpga_slot_id, drm_ctrl_base_addr=base_address)
    assert driver._fpga_slot_id == fpga_slot_id
    assert driver._drm_ctrl_base_addr == base_address
    assert driver.fpga_image is None
    assert driver._fpga_library == library
    assert driver.read_register_callback == Fpga.read_register
    assert driver.write_register_callback == Fpga.write_register
    assert Fpga.fpga_image is None
    assert Fpga.initialized

    # Test: program image with unspecified image
    with pytest.raises(RuntimeError):
        driver.program_fpga()

    # Test: program image
    driver.program_fpga(fpga_image=fpga_image)
    assert driver.fpga_image == fpga_image
    assert Fpga.fpga_image == fpga_image

    # Test instantiate and program image
    Fpga.fpga_image = None
    driver = FpgaDriver(
        fpga_slot_id=fpga_slot_id, drm_ctrl_base_addr=base_address,
        fpga_image=fpga_image)
    assert driver.fpga_image == fpga_image
    assert Fpga.fpga_image == fpga_image

    # Test: Reprogram default image
    Fpga.fpga_image = None
    driver.program_fpga()
    assert Fpga.fpga_image == fpga_image

    # Test Write and read register
    driver.write_register(register_offset=0x12, register_value=1)
    assert Fpga.register[0x12] == 1
    assert driver.read_register(register_offset=0x12) == 1


@pytest.mark.hwtst
def test_get_driver():
    """
    Test accelize_drm.fpga_drivers.get_driver.
    """
    from tests.fpga_drivers import get_driver

    from tests.fpga_drivers._aws_f1 import FpgaDriver as AwsDriver
    assert get_driver('aws_f1') is AwsDriver

    from tests.fpga_drivers._aws_xrt import FpgaDriver as XrtDriver
    assert get_driver('aws_xrt') is XrtDriver


@pytest.mark.hwtst
def test_fpga_driver(accelize_drm, cred_json, conf_json):
    """
    Test the driver used to perform tests.
    """
    from random import randint

    for driver in accelize_drm.pytest_fpga_driver:

        # Test DRM manager instantiation with driver
        drm_manager = accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback, driver.write_register_callback)

        # Tests driver callbacks by writing/reading random values in a register
        for i in range(10):
            new_value = randint(0, 2**32 - 1)
            drm_manager.set(custom_field=new_value)
            assert drm_manager.get('custom_field') == new_value
