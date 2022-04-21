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
from os.path import isfile as _isfile, join as _join, realpath as _realpath, basename as _basename, dirname as _dirname
from re import match as _match
from subprocess import run as _run, PIPE as _PIPE, STDOUT as _STDOUT, Popen
from threading import Lock as _Lock
import sysv_ipc as ipc

from tests.fpga_drivers import FpgaDriverBase as _FpgaDriverBase

__all__ = ['FpgaDriver']


SCRIPT_DIR = _dirname(_realpath(__file__))

SHM_PATH = "/usr/lib/drm-controller/shared-memory-file"


def int_to_bytes(x: int) -> bytes:
    return x.to_bytes(4, 'little', signed=False)

def bytes_to_int(xbytes: bytes) -> int:
    return int.from_bytes(xbytes, 'little', signed=False)


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
        for prefix in (_environ.get("XILINX_XRT", "/opt/xilinx/xrt"),
                       '/usr', '/usr/local'):
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
        if _isfile(_join(xrt_path, 'lib','libxrt_aws.so')):
            print('Loading XRT API library for AWS targets')
            fpga_library = _cdll.LoadLibrary(_join(xrt_path, 'lib','libxrt_aws.so'))
        elif _isfile(_join(xrt_path, 'lib','libxrt_core.so')):
            print('Loading XRT API library for Xilinx targets')
            fpga_library = _cdll.LoadLibrary(_join(xrt_path, 'lib','libxrt_core.so'))
        else:
            raise RuntimeError('Unable to find Xilinx XRT Library')
        return fpga_library

    @staticmethod
    def _get_xbutil():
        xrt_path = FpgaDriver._get_xrt_lib()
        _xbutil_path = _join(xrt_path,'bin','awssak')
        if not _isfile(_xbutil_path):
            _xbutil_path = _join(xrt_path, 'bin','xbutil')
        if not _isfile(_xbutil_path):
            raise RuntimeError('Unable to find Xilinx XRT Board Utility')
        return _xbutil_path

    @property
    def _xbutil(self):
        return self._get_xbutil()

    def _get_lock(self):
        """
        Get a lock on the FPGA driver
        """
        return _Lock

    def _clear_fpga(self):
        """
        Clear FPGA
        """
        '''
        clear_fpga = _run(
            ['fpga-clear-local-image', '-S', str(self._fpga_slot_id)],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if clear_fpga.returncode:
            raise RuntimeError(clear_fpga.stdout)
        '''
        print('FPGA cleared')

    def _program_fpga(self, fpga_image):
        """
        Program the FPGA with the specified image.

        Args:
            fpga_image (str): FPGA image.
        """
        '''
        # Vitis does not reprogram a FPGA that has already the bitstream.
        # So to force it we write another bitstream first.
        clear_image = _join(SCRIPT_DIR, 'clear.awsxclbin')
        load_image = _run(
            [self._xbutil, 'program',
             '-d', str(self._fpga_slot_id), '-p', clear_image],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if load_image.returncode:
            raise RuntimeError(load_image.stdout)

        # Now load the real image
        fpga_image = _realpath(_fsdecode(fpga_image))
        load_image = _run(
            [self._xbutil, 'program',
             '-d', str(self._fpga_slot_id), '-p', fpga_image],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if load_image.returncode:
            raise RuntimeError(load_image.stdout)
        '''
        # Init global specific variables
        self.shm_pages = list()
        self.ctrl_sw_exec = None

        # Start Controller SW
        fpga_image = ''
        if not _isfile(fpga_image):
            pass
#            raise RuntimeError('Controller SW executable path is invald: ', fpga_image)
#        self.ctrl_sw_exec = Popen([self.ctrl_sw_exec, self._fpga_image], shell=False, stdout=_PIPE, stderr=_STDOUT)
        print('Programmed AWS SoM with', self.ctrl_sw_exec)

    def _reset_fpga(self):
        """
        Reset FPGA including FPGA image.
        """
        '''
        reset_image = _run(
            [self._xbutil, 'reset', '-d',
             str(self._fpga_slot_id)],
            stderr=_STDOUT, stdout=_PIPE, universal_newlines=True, check=False)
        if reset_image.returncode:
            raise RuntimeError(reset_image.stdout)
        '''
        pass

    def _init_fpga(self):
        """
        Initialize FPGA handle with driver library.
        """
        '''
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
        '''
        # Connect to Shared Memory
        self.shm_pages = list()
        for i in range(1,7):
            key = ipc.ftok(SHM_PATH, i)
            shm = ipc.SharedMemory(key, 0, 0)
            shm.attach(0, 0)
            self.shm_pages.append(shm)
        print('Connected to Shared Memory')

    def _uninit_fpga(self):
        import signal
        # Release access to shared memory
        for shm in self.shm_pages:
            shm.detach()
        print('Disconnected from Shared Memory')
        if self.ctrl_sw_exec:
            self.ctrl_sw_exec.send_signal(signal.SIGINT)
            self.ctrl_sw_exec.wait()
        print('Terminate DRM')

    def int_to_bytes(x: int) -> bytes:
        return x.to_bytes((x.bit_length() + 7) // 8, 'little', signed=False)

    def int_from_bytes(xbytes: bytes) -> int:
        return int.from_bytes(xbytes, 'big')

    def _get_read_register_callback(self):
        """
        Read register callback.

        Returns:
            function: Read register callback
        """
        '''
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
        '''
        def read_register(register_offset, returned_data, driver=self):
            """
            Read register.

            Args:
                register_offset (int): Offset
                returned_data (int pointer): Return data.
                driver (accelize_drm.fpga_drivers._aws_xrt.FpgaDriver):
                    Keep a reference to driver.
            """
            with driver._fpga_register_lock():
                if register_offset >= 0x10000:
                    '''
                    size_or_error = driver._fpga_read_register(
                        driver._fpga_handle,
                        2,  # XCL_ADDR_KERNEL_CTRL
                        driver._drm_ctrl_base_addr + register_offset,
                        returned_data,
                        4  # 4 bytes
                    )
                    ret = size_or_error if size_or_error != 4 else 0
                    '''
                    return 0
                    raise RuntimeError('Reading from Activator is not supported in Emu-HW')
                else:
                    page_index = bytes_to_int(driver.shm_pages[0].read(4, 0))
                    if register_offset == 0:
                        reg_value = page_index
                    else:
                        shm = driver.shm_pages[page_index]
                        reg_value = bytes_to_int(shm.read(4, register_offset))
#                    print('Read @%08X: 0x%08X' % (register_offset, reg_value))
                    returned_data.contents.value = reg_value
                    ret = 0
            return ret

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
            with driver._fpga_register_lock():
                if register_offset >= 0x10000:
                    '''
                    size_or_error = driver._fpga_write_register(
                        driver._fpga_handle,
                        2,  # XCL_ADDR_KERNEL_CTRL
                        driver._drm_ctrl_base_addr + register_offset,
                        data_to_write.to_bytes(4, byteorder="little"),
                        4  # 4 bytes
                    )
                    ret = size_or_error if size_or_error != 4 else 0
                    '''
                    return 0
                    raise RuntimeError('Writing to Activator is not supported in Emu-HW')
                else:
                    page_index = bytes_to_int(driver.shm_pages[0].read(4, 0))
                    if register_offset == 0:
                        driver.shm_pages[0].write(int_to_bytes(data_to_write), 0)
                    else:
                        shm = driver.shm_pages[page_index]
                        shm.write(int_to_bytes(data_to_write), register_offset)
#                    print('Wrote @%08X: 0x%08X' % (register_offset, data_to_write))
                    ret = 0
            # Return 0 return code if written size else return error code
            return ret

        return write_register
