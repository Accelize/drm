#! /usr/bin/env python3
# coding=utf-8
"""
AWS F1 driver for Accelize DRM Python library

Example:

    # Get Driver
    from tests.fpga_drivers import get_driver
    driver = get_driver('aws_f1')()

    # Instantiate DrmManager with driver callbacks
    import accelize_drm

    drm_manager = accelize_drm.DrmManager(
       './conf.json', './cred.json',
       driver.read_register_callback,
       driver.write_register_callback)
"""
from abc import abstractmethod as _abstractmethod
from importlib import import_module as _import_module


def get_driver(name):
    """
    Get a driver by name.

    Args:
        name (str): Driver name.

    Returns:
        FpgaDriverBase subclass: driver class.
    """
    return getattr(_import_module(f'{__name__}.{name}'), 'FpgaDriver')


class FpgaDriverBase:
    """
    Base class for FPGA driver to use with accelize_drm.DrmManager.

    Args:
        drm_ctrl_base_addr (int): DRM Controller base address.
        slot_id (int): FPGA slot ID.
    """
    #: FPGA slot
    SLOT_ID = 0

    #: Acelize DRM Controller base address
    DRM_CONTROLLER_BASE_ADDR = 0

    #: Accelize FPGA image/bitstream to use for test
    FPGA_IMAGE = None

    def __init__(self, init_fpga=True):
        # Device and library handles
        self._fpga_handle = None
        self._fpga_library = self._get_driver()

        # Initialize FPGA
        if init_fpga:
            self._init_fpga()

    def init_fpga(self):
        """
        Initialize FPGA
        """
        self._init_fpga()

    @property
    def read_register_callback(self):
        """
        Read register callback.

        Returns:
            function: Callback
        """
        return self._get_read_register_callback()

    @property
    def write_register_callback(self):
        """
        Write register callback.

        Returns:
            function: Callback
        """
        return self._get_write_register_callback()

    @_abstractmethod
    def _get_driver(self):
        """
        Get FPGA driver

        Returns:
            object: FPGA driver.
        """

    @_abstractmethod
    def _init_fpga(self):
        """
        This function should initialize FPGA and performs checks to verify
        everything is good with loaded design, driver, ...
        """

    @_abstractmethod
    def _get_read_register_callback(self):
        """
        Read register callback.

        The function needs to return an int and accept following arguments:
        register_offset (int), returned_data (int).

        The function can't be a non static method.

        Returns:
            function: Read register callback
        """

    @_abstractmethod
    def _get_write_register_callback(self):
        """
        Write register callback.

        The function needs to return an int and accept following arguments:
        register_offset (int), data_to_write (int).

        The function can't be a non static method.

        Returns:
            function: Write register callback
        """
