# coding=utf-8
"""
Xilinx XRT driver for Accelize DRM Python library

Requires XRT: https://github.com/Xilinx/XRT
"""
from ctypes import (
    cdll as _cdll, POINTER as _POINTER, c_char_p as _c_char_p,
    c_uint as _c_uint, c_uint64 as _c_uint64, c_int as _c_int,
    c_void_p as _c_void_p, c_size_t as _c_size_t)
from os import environ as _environ, fsdecode as _fsdecode
from os.path import isfile as _isfile, join as _join, realpath as _realpath, basename as _basename
from re import match as _match
from subprocess import run as _run, PIPE as _PIPE, STDOUT as _STDOUT
from threading import Lock as _Lock

from tests.fpga_drivers import FpgaDriverBase as _FpgaDriverBase

__all__ = ['FpgaDriver']


class XrtLock():
    def __init__(self, driver):
        self.driver = driver
        # Define Lock function
        self.xcl_lock = driver._fpga_library.xclLockDevice
        self.xcl_lock.restype = _c_int
        self.xcl_lock.argtype = _c_void_p
        # Define Unlock function
        self.xcl_unlock = driver._fpga_library.xclLockDevice
        self.xcl_unlock.restype = _c_int
        self.xcl_unlock.argtype = _c_void_p

    def __enter__(self):
        if self.driver._fpga_handle is None:
            raise RuntimeError('Null device handle')
        self.driver._reglock.acquire()
        self.xcl_lock(self.driver._fpga_handle)
        return

    def __exit__(self, *kargs):
        if self.driver._fpga_handle is None:
            raise RuntimeError('Null device handle')
        self.xcl_unlock(self.driver._fpga_handle)
        self.driver._reglock.release()


class FpgaDriver(_FpgaDriverBase):
    """
    Generates functions to use XRT with accelize_drm.DrmManager.

    Args:
        fpga_slot_id (int): FPGA slot ID.
        fpga_image (str): Path to ".xclbin" binary to use to program FPGA.
        drm_ctrl_base_addr (int): DRM Controller base address.
        log_dir (path-like object): directory where XRT will output log file.
    """
    _name = _match(r'_(.+)\.py', _basename(__file__)).group(1)
    _reglock = _Lock()

    def _get_driver(self):
        """
        Get FPGA driver

        Returns:
            ctypes.CDLL: FPGA driver.
        """
        if _isfile(_join(self._xrt_prefix, "lib/libxrt_aws.so")):
            print('Loading XRT for AWS target'
            fpga_library = _cdll.LoadLibrary(_join(self._xrt_prefix, "lib/libxrt_aws.so"))
        elif _isfile(_join(self._xrt_prefix, "lib/libxrt_core.so")):
            print('Loading XRT for common target'
            fpga_library = _cdll.LoadLibrary(_join(self._xrt_prefix, "lib/libxrt_core.so"))
        else:
            raise RuntimeError('Unable to find Xilinx XRT Library')
        return fpga_library

    def _get_lock(self):
        """
        Get a lock on the FPGA driver
        """
        def create_lock():
            return XrtLock(self)
        return create_lock

    @property
    def _xrt_prefix(self):
        """
        Detect XRT installation prefix:

        Returns:
            str: prefix path.
        """
        for prefix in (_environ.get("XILINX_XRT", "/opt/xilinx/xrt"),
                       '/usr', '/usr/local'):
            if _isfile(_join(prefix, 'bin/xbutil')):
                return prefix
        raise RuntimeError('Unable to find Xilinx XRT')

    @property
    def _xbutil(self):
        _xbutil_path = _join(self._xrt_prefix, 'bin/awssak')
        if not _isfile(_xbutil_path):
            _xbutil_path = _join(self._xrt_prefix, 'bin/xbutil')
        if not _isfile(_xbutil_path):
            raise RuntimeError('Unable to find Xilinx XRT Board Utility')
        return _xbutil_path

    def _clear_fpga(self):
        """
        Clear FPGA
        """
        clear_fpga = _run(
            ['fpga-clear-local-image', '-S', str(self._fpga_slot_id)],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if clear_fpga.returncode:
            raise RuntimeError(clear_fpga.stdout)

    def _program_fpga(self, fpga_image):
        """
        Program the FPGA with the specified image.

        Args:
            fpga_image (str): FPGA image.
        """
        fpga_image = _realpath(_fsdecode(fpga_image))
        load_image = _run(
            [self._xbutil, 'program',
             '-d', str(self._fpga_slot_id), '-p', fpga_image],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if load_image.returncode:
            raise RuntimeError(load_image.stdout)
        print('Programmed AWS F1 slot #%d with FPGA image %s' % (self._fpga_slot_id, fpga_image))

    def _reset_fpga(self):
        """
        Reset FPGA including FPGA image.
        """
        reset_image = _run(
            [self._xbutil, 'reset', '-d',
             str(self._fpga_slot_id)],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if reset_image.returncode:
            raise RuntimeError(reset_image.stdout)

    def _init_fpga(self):
        """
        Initialize FPGA handle with driver library.
        """
        # Find all devices
        xcl_probe = self._fpga_library.xclProbe
        xcl_probe.restype = _c_uint  # Devices count

        if xcl_probe() < 1:
            raise RuntimeError("xclProbe does not found devices")

        # Open device
        xcl_open = self._fpga_library.xclOpen
        xcl_open.restype = _POINTER(_c_void_p)  # Device handle
        xcl_open.argtypes = (
            _c_uint,  # deviceIndex
            _c_char_p,  # logFileName
            _c_int,  # level
        )
        log_file = _join(self._log_dir, 'slot_%d_xrt.log' % self._fpga_slot_id)
        device_handle = xcl_open(
            self._fpga_slot_id,
            log_file.encode(),
            3  # XCL_ERROR
        )
        if not device_handle:
            raise RuntimeError("xclOpen failed to open device")
        self._fpga_handle = device_handle

    def _get_read_register_callback(self):
        """
        Read register callback.

        Returns:
            function: Read register callback
        """
        xcl_read = self._fpga_library.xclRead
        xcl_read.restype = _c_size_t  # read size or error code
        xcl_read.argtypes = (
            _c_void_p,  # handle
            _c_int,  # space
            _c_uint64,  # offset
            _c_void_p,  # hostBuf
            _c_size_t  # size
        )
        self._fpga_read_register = xcl_read

        def read_register(register_offset, returned_data, driver=self):
            """
            Read register.

            Args:
                register_offset (int): Offset
                returned_data (int pointer): Return data.
                driver (accelize_drm.fpga_drivers._xilinx_xrt.FpgaDriver):
                    Keep a reference to driver.
            """
            with driver._fpga_read_register_lock():
                size_or_error = driver._fpga_read_register(
                    driver._fpga_handle,
                    2,  # XCL_ADDR_KERNEL_CTRL
                    driver._drm_ctrl_base_addr + register_offset,
                    returned_data,
                    4  # 4 bytes
                )

            # Return 0 return code if read size else return error code
            return size_or_error if size_or_error != 4 else 0

        return read_register

    def _get_write_register_callback(self):
        """
        Write register callback.

        Returns:
            function: Write register callback
        """
        xcl_write = self._fpga_library.xclWrite
        xcl_write.restype = _c_size_t  # written size or error code
        xcl_write.argtypes = (
            _c_void_p,  # handle
            _c_int,  # space
            _c_uint64,  # offset
            _c_char_p,  # hostBuf
            _c_size_t  # size
        )
        self._fpga_write_register = xcl_write

        def write_register(register_offset, data_to_write, driver=self):
            """
            Write register.

            Args:
                register_offset (int): Offset
                data_to_write (int): Data to write.
                driver (accelize_drm.fpga_drivers._xilinx_xrt.FpgaDriver):
                    Keep a reference to driver.
            """
            with driver._fpga_write_register_lock():
                size_or_error = driver._fpga_write_register(
                    driver._fpga_handle,
                    2,  # XCL_ADDR_KERNEL_CTRL
                    driver._drm_ctrl_base_addr + register_offset,
                    data_to_write.to_bytes(4, byteorder="little"),
                    4  # 4 bytes
                )

            # Return 0 return code if written size else return error code
            return size_or_error if size_or_error != 4 else 0

        return write_register
