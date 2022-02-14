# coding=utf-8
"""
Xilinx XRT driver for Accelize DRM Python library

Requires XRT: https://github.com/Xilinx/XRT
"""
import pyopencl as cl
from ctypes import (
    cdll as _cdll, POINTER as _POINTER, c_char_p as _c_char_p,
    c_uint as _c_uint, c_uint64 as _c_uint64, c_int as _c_int,
    c_void_p as _c_void_p, c_size_t as _c_size_t)
from os import environ as _environ, fsdecode as _fsdecode
from os.path import isfile as _isfile, join as _join, realpath as _realpath, basename as _basename, dirname as _dirname, splitext
from re import match as _match, search
from subprocess import run as _run, PIPE as _PIPE, STDOUT as _STDOUT, Popen
from threading import Lock as _Lock

from tests.fpga_drivers import FpgaDriverBase as _FpgaDriverBase

__all__ = ['FpgaDriver']


SCRIPT_DIR = _dirname(_realpath(__file__))


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

    @staticmethod
    def _get_xrt_lib():
        """
        Detect XRT installation path:
        """
        prefix = '/usr'
        if _isfile(_join(prefix, 'bin','xbutil')):
            return prefix
        raise RuntimeError('Unable to find Xilinx XRT')

    @staticmethod
    def _get_driver():
        """
        Get FPGA driver

        Returns:
            ctypes.CDLL: FPGA driver.
        """
        xrt_path = FpgaDriver._get_xrt_lib()
        if _isfile(_join(xrt_path, 'lib','libxrt_core.so')):
            print('Loading XRT API library for SoM targets')
            fpga_library = _cdll.LoadLibrary(_join(xrt_path, 'lib','libxrt_core.so'))
        else:
            raise RuntimeError('Unable to find Xilinx XRT Library')
        return fpga_library

    @staticmethod
    def _get_xbutil():
        xrt_path = FpgaDriver._get_xrt_lib()
        _xbutil_path = _join(xrt_path, 'bin','xmutil')
        if not _isfile(_xbutil_path):
            raise RuntimeError('Unable to find Xilinx XRT Board Utility')
        return _xbutil_path

    @property
    def _xmutil(self):
        return self._get_xbutil()

    def _get_lock(self):
        """
        Get a lock on the FPGA driver
        """
        def create_lock():
            return XrtLock(self)
        return create_lock

    def _clear_fpga(self):
        """
        Clear FPGA
        """
        clear_fpga = _run(
            [self._xmutil, 'unloadapp'],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if clear_fpga.returncode:
            raise RuntimeError(clear_fpga.stdout)
        print('FPGA cleared')

    def _program_fpga(self, fpga_image):
        """
        Program the FPGA with the specified image.

        Args:
            fpga_image (str): FPGA image.
        """
        # Must clear the FPGA first
        #self._clear_fpga()

        # Now load the bitstream
        image_name = splitext(fpga_image)[0]
        load_image = _run(
            [self._xmutil, 'loadapp', image_name],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if load_image.returncode or search(r'error', load_image.stdout):
            raise RuntimeError(load_image.stdout)
        print('Programmed SoM XRT with FPGA image %s' % fpga_image)

    def _reset_fpga(self):
        """
        Reset FPGA including FPGA image.
        """
        list_apps = _run(
            [self._xmutil, 'listapps'],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if list_apps.returncode or search(r'error', list_apps.stdout):
            raise RuntimeError(list_apps.stdout)
        image_name = ''
        for line in list_apps.stdout.splitlines():
            m = _match(r'\s*(.+?)\s+', line)
            if m:
                image_name = m.group(1)
        if not image_name:
            print('No loaded bitstream to reset')
            return
        self._program_fpga(image_name + ".som")
        
    def _init_fpga(self):
        """
        Initialize FPGA handle with XRT and OpenCL libraries.
        """
        image_name = splitext(self._fpga_image)[0]
        dev =cl.get_platforms()[0].get_devices()
        binary = open(_join('/lib','firmware','xilinx',image_name,image_name+'.xclbin'), 'rb').read()
        ctx = cl.Context(dev_type=cl.device_type.ALL)
        prg = cl.Program(ctx,dev,[binary])
        prg.build()

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


    def _uninit_fpga(self):
        pass

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
                driver (accelize_drm.fpga_drivers._aws_xrt.FpgaDriver):
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
                driver (accelize_drm.fpga_drivers._aws_xrt.FpgaDriver):
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