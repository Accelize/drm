# -*- coding: utf-8 -*-
"""
Test Python library.
"""
import pytest


def test_raise_exceptions(accelize_drm):
    """
    Test proper Exception raising.
    """
    exc = accelize_drm.exceptions

    # Test: Raise function and default async callback
    for raise_from_error in (exc._raise_from_error, exc._async_error_callback):

        # Raise an exception from message with error code
        with pytest.raises(exc.DRMFatal):
            raise_from_error('message [errCode=30001]\n')

        # Raise an exception with no error code
        with pytest.raises(exc.DRMException):
            raise_from_error('message')

    # Tests: Raise an exception from error code
    with pytest.raises(exc.DRMWSRespError):
        exc._raise_from_error('message', error_code=10001)


def test_get_api_version(accelize_drm):
    """
    Test get_api_version function.
    """
    # Mock Versions
    accelize_drm___version__ = accelize_drm.__version__
    accelize_drm__get_api_version = accelize_drm._get_api_version

    # Tests
    try:
        # Short version
        py_version = '1.2.3'
        version = '4.5.6'
        accelize_drm.__version__ = py_version
        accelize_drm._get_api_version = lambda: version.encode()
        versions = accelize_drm.get_api_version()
        assert versions.py_major == 1
        assert versions.py_minor == 2
        assert versions.py_revision == 3
        assert versions.py_prerelease == ''
        assert versions.py_build == ''
        assert versions.py_version == py_version
        assert versions.major == 4
        assert versions.minor == 5
        assert versions.revision == 6
        assert versions.prerelease == ''
        assert versions.build == ''
        assert versions.version == version

        # Prerelease version with build
        py_version = '1.2.3-alpha.1+commit1'
        version = '4.5.6-beta.2+commit2'
        accelize_drm.__version__ = py_version
        accelize_drm._get_api_version = lambda: version.encode()
        versions = accelize_drm.get_api_version()
        assert versions.py_major == 1
        assert versions.py_minor == 2
        assert versions.py_revision == 3
        assert versions.py_prerelease == 'alpha.1'
        assert versions.py_build == 'commit1'
        assert versions.py_version == py_version
        assert versions.major == 4
        assert versions.minor == 5
        assert versions.revision == 6
        assert versions.prerelease == 'beta.2'
        assert versions.build == 'commit2'
        assert versions.version == version

    # Restore mocked versions
    finally:
        accelize_drm.__version__ = accelize_drm___version__
        accelize_drm._get_api_version = accelize_drm__get_api_version


def test_fpga_drivers_base():
    """
    Test accelize_drm.fpga_drivers.FpgaDriverBase.
    """
    from accelize_drm.fpga_drivers import FpgaDriverBase
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


def test_get_driver():
    """
    Test accelize_drm.fpga_drivers.get_driver.
    """
    from accelize_drm.fpga_drivers import get_driver

    # Test driver import.
    from accelize_drm.fpga_drivers._aws_f1 import FpgaDriver
    assert get_driver('aws_f1') is FpgaDriver
