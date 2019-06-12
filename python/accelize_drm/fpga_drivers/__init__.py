# coding=utf-8
"""
FPGA driver for Accelize DRM Python library

Supported driver:
    AWS F1 instances types
        Require ``libfpga_mgmt`` library from AWS FPGA SDK
        (https://github.com/aws/aws-fpga).

"""
from abc import abstractmethod as _abstractmethod
from ctypes import c_uint32 as _c_uint32, byref as _byref
from importlib import import_module as _import_module
from threading import Lock as _Lock

__all__ = ['get_driver', 'FpgaDriverBase']


def get_driver(name):
    """
    Get a driver by name.

    Args:
        name (str): Driver name. Possible values:
            `aws_f1` (AWS F1 instances types).

    Returns:
        FpgaDriverBase subclass: driver class.
    """
    return getattr(_import_module('%s._%s' % (__name__, name)), 'FpgaDriver')


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

        # FPGA read/write low level functions ans associated locks
        self._fpga_read_register = None
        self._fpga_write_register = None
        self._fpga_read_register_lock = _Lock()
        self._fpga_write_register_lock = _Lock()

        # Device and library handles
        self._fpga_handle = None
        self._fpga_library = self._get_driver()

        # Initialize FPGA
        if fpga_image:
            self._program_fpga(fpga_image)
        self._init_fpga()

        # Call backs
        self._read_register_callback = self._get_read_register_callback()
        self._write_register_callback = self._get_write_register_callback()

    @property
    def fpga_image(self):
        """
        Programmed FPGA image.

        Returns:
            str: FPGA image.
        """
        return self._fpga_image

    def read_register(self, register_offset):
        """
        Read register.

        This method is intended to be called directly, use
        "read_register_callback" to instantiate
        "accelize_drm.DrmManager".

        Args:
            register_offset (int): Register offset.

        Returns:
            int: 32 bits register value.
        """
        register_value = _c_uint32(0)
        self._read_register_callback(register_offset, _byref(register_value))
        return register_value.value

    @property
    def read_register_callback(self):
        """
        Read register callback to pass to "accelize_drm.DrmManager".

        This method is not intended to be used directly,
        se "read_register" method for direct call.

        Returns:
            function: Callback
        """
        return self._read_register_callback

    def write_register(self, register_offset, register_value):
        """
        Write register.

        This method is intended to be called directly, use
        "write_register_callback" to instantiate
        "accelize_drm.DrmManager".

        Args:
            register_offset (int): Register offset.
            register_value (int): 32 bits register value to write.
        """
        self.write_register_callback(register_offset, register_value)

    @property
    def write_register_callback(self):
        """
        Write register callback to pass to "accelize_drm.DrmManager".

        This method is not intended to be used directly,
        se "write_register" method for direct call.

        Returns:
            function: Callback
        """
        return self._write_register_callback

    def program_fpga(self, fpga_image=None):
        """
        Program FPGA with specified image.

        Args:
            fpga_image (str): FPGA image to program.
        """
        if fpga_image is None:
            if not self._fpga_image:
                raise RuntimeError('Unspecified fpga_image')
            fpga_image = self._fpga_image
        else:
            self._fpga_image = fpga_image

        print('Programming FPGA with image ID: ', fpga_image)
        return self._program_fpga(fpga_image)

    @_abstractmethod
    def _get_driver(self):
        """
        Get FPGA driver.

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
        register_offset (int), returned_data (int pointer).

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
