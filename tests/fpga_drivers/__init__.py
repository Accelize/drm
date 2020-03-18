# coding=utf-8
"""
FPGA driver for Accelize DRM Python library

Supported driver:
    AWS F1 instances types
        Require ``libfpga_mgmt`` library from AWS FPGA SDK
        (https://github.com/aws/aws-fpga).
    Xilinx XRT
        Require XRT (https://github.com/Xilinx/XRT).

"""
from abc import abstractmethod as _abstractmethod
from contextlib import contextmanager
from ctypes import c_uint32 as _c_uint32, byref as _byref
from importlib import import_module as _import_module
from os import fsdecode as _fsdecode
from os.path import realpath as _realpath

__all__ = ['get_driver', 'FpgaDriverBase']


def get_driver(name):
    """
    Get a driver by name.

    Args:
        name (str): Driver name. Possible values:
            `aws_f1` (AWS F1 instances types),
            `xilinx_xrt` (Xilinx XRT).

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
        log_dir (path-like object): Some drivers may output logs files in this
            directory. Default to current directory.
    """

    def __init__(self, fpga_slot_id=0, fpga_image=None, drm_ctrl_base_addr=0,
                 log_dir='.', no_clear_fpga=False):
        self._fpga_slot_id = fpga_slot_id
        self._fpga_image = fpga_image
        self._drm_ctrl_base_addr = drm_ctrl_base_addr
        self._log_dir = _realpath(_fsdecode(log_dir))

        # FPGA read/write low level functions ans associated locks
        self._fpga_read_register = None
        self._fpga_write_register = None
        self._fpga_read_register_lock = self._get_lock()
        self._fpga_write_register_lock = self._get_lock()

        if not no_clear_fpga:
           self.clear_fpga()

        # Device and library handles
        self._fpga_handle = None
        self._fpga_library = self._get_driver()

        # Initialize FPGA
        if fpga_image:
            self.program_fpga(fpga_image)

        with self._augment_exception('initialize'):
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
        if self._read_register_callback(register_offset, _byref(register_value)):
            raise RuntimeError('Failed to read register at offset %08X' % register_offset)
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
        if self.write_register_callback(register_offset, register_value):
            raise RuntimeError('Failed to write register at offset %08X' % register_offset)

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

    def clear_fpga(self):
        """
        Clear FPGA including FPGA image.
        """
        print('Clearing FPGA on slot #%d' % self._fpga_slot_id)
        with self._augment_exception('clear'):
            return self._clear_fpga()

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

        print('Programming FPGA on slot #%d with FPGA image %s' % (self._fpga_slot_id, fpga_image))
        with self._augment_exception('program'):
            return self._program_fpga(fpga_image)

    def reset_fpga(self):
        """
        Reset FPGA including FPGA image.
        """
        with self._augment_exception('reset'):
            return self._reset_fpga()

    @staticmethod
    @contextmanager
    def _augment_exception(action):
        """
        Augment exception message.

        Args:
            action (str): Action performed on FPGA.
        """
        try:
            yield
        except RuntimeError as exception:
            exception.args = (
                'Unable to %s FPGA: %s' % (action, exception.args[0].strip()),)
            raise

    @_abstractmethod
    def _get_driver(self):
        """
        Get FPGA driver.

        Returns:
            object: FPGA driver.
        """

    @_abstractmethod
    def _get_lock(self):
        """
        Get FPGA driver lock.

        Returns:
            object: FPGA driver lock.
        """

    @_abstractmethod
    def _clear_fpga(self):
        """
        Clear the FPGA.
        """

    @_abstractmethod
    def _program_fpga(self, fpga_image):
        """
        Program the FPGA with the specified image.

        Args:
            fpga_image (str): FPGA image.
        """

    @_abstractmethod
    def _reset_fpga(self):
        """
        Reset FPGA including FPGA image.
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
