#! /usr/bin/env python3
# coding=utf-8
"""
FPGA driver for Accelize DRM Python library

Example:

    # Get Driver by name (Example with AWS F1 driver)
    from tests.python_fpga_drivers import get_driver
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
        fpga_slot_id (int): FPGA slot ID.
        fpga_image (str): FPGA image to use to program FPGA.
        drm_ctrl_base_addr (int): DRM Controller base address.
    """

    def __init__(self, fpga_slot_id=0, fpga_image=None, drm_ctrl_base_addr=0):
        self._fpga_slot_id = fpga_slot_id
        self._fpga_image = fpga_image
        self._drm_ctrl_base_addr = drm_ctrl_base_addr

        # Device and library handles
        self._fpga_handle = None
        self._fpga_library = self._get_driver()

        # Initialize FPGA
        if fpga_image:
            self._program_fpga(fpga_image)
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
    def _program_fpga(self, fpga_image):
        """
        Program the FPGA with the specified image.

        Args:
            fpga_image (str): FPGA image.
        """

    @_abstractmethod
    def _init_fpga(self):
        """
        Initialize FPGA handle with driver library.
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
