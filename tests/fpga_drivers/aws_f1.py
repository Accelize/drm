#! /usr/bin/env python3
# coding=utf-8
"""
AWS F1 driver for Accelize DRM Python library
"""
from tests.fpga_drivers import FpgaDriverBase as _FpgaDriverBase
from ctypes import (
    cdll as _cdll, POINTER as _POINTER, byref as _byref, c_uint32 as _c_uint32,
    c_uint64 as _c_uint64, c_int as _c_int)
from subprocess import run as _run, PIPE as _PIPE, STDOUT as _STDOUT


class FpgaDriver(_FpgaDriverBase):
    """
    Generates functions to use AWS FPGA F1 with accelize_drm.DrmManager.

    Args:
        drm_ctrl_base_addr (int): DRM Controller base address.
        slot_id (int): FPGA slot ID.
    """

    #: Accelize AGFI to use for test
    FPGA_IMAGE = 'agfi-03be02f29cd7e466e'

    def _get_driver(self):
        """
        Get FPGA driver

        Returns:
            ctypes.CDLL: FPGA driver.
        """
        # Load AWS FPGA library
        # See AWS FPGA SDK: https://github.com/aws/aws-fpga
        fpga_library = _cdll.LoadLibrary("libfpga_mgmt.so")

        fpga_pci_init = fpga_library.fpga_pci_init
        fpga_pci_init.restype = _c_int  # return code
        if fpga_pci_init():
            raise RuntimeError('Unable to initialize the "fpga_pci" library')

        # Default FPGA handle
        self._fpga_handle = _c_int(-1)  # PCI_BAR_HANDLE_INIT

        return fpga_library

    def _init_fpga(self):
        """
        Initialize FPGA
        """
        # Load FPGA image
        load_image = _run(['fpga-load-local-image', f'-S {self.SLOT_ID}', '-I',
                           self.FPGA_IMAGE], stderr=_STDOUT, stdout=_PIPE,
                          universal_newlines=True)
        if load_image.returncode:
            raise RuntimeError(load_image.stdout)

        # Reset FPGA handle
        self._fpga_handle = _c_int(-1)  # PCI_BAR_HANDLE_INIT

        # Attach FPGA
        fpga_pci_attach = self._fpga_library.fpga_pci_attach
        fpga_pci_attach.restype = _c_int  # return code
        fpga_pci_attach.argtypes = (
            _c_int,  # slot_id
            _c_int,  # pf_id
            _c_int,  # bar_id
            _c_uint32,  # flags
            _POINTER(_c_int)  # handle
        )

        if fpga_pci_attach(self.SLOT_ID,
                           0,  # FPGA_APP_PF
                           0,  # APP_PF_BAR0
                           0, _byref(self._fpga_handle)):
            raise RuntimeError(
                f"Unable to attach to the AFI on slot ID {self.SLOT_ID}")

    def _get_read_register_callback(self):
        """
        Read register callback.

        Returns:
            function: Read register callback
        """
        fpga_pci_peek = self._fpga_library.fpga_pci_peek
        fpga_pci_peek.restype = _c_int  # return code
        fpga_pci_peek.argtypes = (
            _c_int,  # handle
            _c_uint64,  # offset
            _POINTER(_c_uint32)  # value
        )

        base_addr = self.DRM_CONTROLLER_BASE_ADDR
        handle = self._fpga_handle

        return lambda register_offset, returned_data: fpga_pci_peek(
            handle, base_addr + register_offset, returned_data)

    def _get_write_register_callback(self):
        """
        Write register callback.

        Returns:
            function: Write register callback
        """
        fpga_pci_poke = self._fpga_library.fpga_pci_poke
        fpga_pci_poke.restype = _c_int  # return code
        fpga_pci_poke.argtypes = (
            _c_int,  # handle
            _c_uint64,  # offset
            _c_uint32  # value
        )

        base_addr = self.DRM_CONTROLLER_BASE_ADDR
        handle = self._fpga_handle

        return lambda register_offset, data_to_write: fpga_pci_poke(
            handle, base_addr + register_offset, data_to_write)
