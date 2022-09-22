# -*- coding: utf-8 -*-
"""
Test all other vitis reference designs
"""
import pytest
from ctypes import c_uint, byref
from json import loads
from random import randint
from re import match


RegBridgeVersion = 0x0
RegBridgeStream = 0x4
RegBridgeMailboxSize = 0x8
RegBridgeMailboxRoData = 0xC

DRM_BRIDGE_VERSION_EXPECTED = 0x02000000


def reverse_string(x):
    y=list(x)
    y.reverse()
    y_nozero = [e for e in y if e != '\x00']
    return ''.join(y_nozero)


def stringify(h):
    h_str = bytes.fromhex(h).decode('utf-8')
    if len(h_str) % 4:
        raise Exception('Hex string is not multiple of 4')
    h_list = [h_str[y-4:y] for y in range(4, len(h_str)+4,4)]
    h_list = list(map(reverse_string, h_list))
    return ''.join(h_list)


def run_test_on_design(accelize_drm, design_name, is_dual_clk, num_activator_expected=1):
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test skipped on SoM target because it's meant to test the DRM bridge only on AWS (without DRM Sw)")

    # Program board with design
    ref_designs = accelize_drm.pytest_ref_designs
    try:
        fpga_image = ref_designs.get_image_id(design_name)
    except:
        pytest.skip(f"Could not find refesign name '{design_name}' for driver '{accelize_drm.pytest_fpga_driver_name}'")
    driver = accelize_drm.pytest_fpga_driver[0]
    driver.program_fpga(fpga_image)
    accelize_drm.scanActivators()

    # Prepare tests
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    # Test controller bridge version
    reg = c_uint(0)
    assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeVersion, byref(reg)) == 0
    assert reg.value == DRM_BRIDGE_VERSION_EXPECTED

    # Test controller bridge mailbox size
    assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeMailboxSize, byref(reg)) == 0
    mailbox_ro_size = reg.value >> 16
    mailbox_rw_size = reg.value & 0xFFFF
    assert mailbox_ro_size > 0x10
    assert mailbox_rw_size > 0

    # Test controller bridge mailbox read-only content
    hex_str = ''
    for i in range(mailbox_ro_size):
        assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeMailboxRoData + 4*i, byref(reg)) == 0
        hex_str += '%08X' % reg.value
    text_str = stringify(hex_str)
    text_json = loads(text_str)
    assert text_json['pkg_version']
    assert text_json['drm_software']
    assert text_json['product_id']
    assert text_json['product_id']['vendor'] == 'accelize.com'
    assert text_json['product_id']['library'] == 'refdesign'
    assert text_json['product_id']['name'] == 'drm_%dactivator' % num_activator_expected
    assert text_json['extra']
    assert text_json['extra']['fpga_family'] == 'random_id'
    assert text_json['extra']['fpga_vendor'] == 'xilinx'
    assert text_json['extra']['csp'].lower() == 'som'
    assert text_json['extra']['dualclk'] == is_dual_clk
    rom_version_str = text_json['extra']['lgdn_full_version']
    assert rom_version_str

    # Compute hex version to be later used
    rom_version_match = match(r'(\d+)\.(\d+)\.(\d+)\.\d+', rom_version_str)
    rom_version = ''.join(map(lambda x: '%02X' % int(x), rom_version_match.groups()))

    # Test controller bridge mailbox read-write content
    regBridgeMailboxRWData = RegBridgeMailboxRoData + 4*mailbox_ro_size
    ref_list = []
    for i in range(mailbox_rw_size):
        ref = randint(1,0xFFFFFFFF)
        assert driver.write_register_callback(driver._drm_ctrl_base_addr + regBridgeMailboxRWData + 4*i, ref) == 0
        ref_list.append(ref)
    for i in range(mailbox_rw_size):
        assert driver.read_register_callback(driver._drm_ctrl_base_addr + regBridgeMailboxRWData + 4*i, byref(reg)) == 0
        assert reg.value == ref_list[i]

    # Reset stream counter
    assert driver.write_register_callback(driver._drm_ctrl_base_addr + RegBridgeVersion, 1) == 0

    # Discover number of activators on the daisy chain
    assert driver.write_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, 0x100100FF) == 0
    assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, byref(reg)) == 0     # Read TID
    num_activator_detected = 0xFF - reg.value
    assert num_activator_detected == num_activator_expected
    assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, byref(reg)) == 0     # Read dummy write command

    for i in range(num_activator_detected):
        # Get Activator's LGDN version through bridge
        assert driver.write_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, 0x22010000+i) == 0
        assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, byref(reg)) == 0 # Read TID
        assert reg.value == 0xFF
        assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, byref(reg)) == 0
        assert reg.value == int(rom_version,16) << 8

        # Get Activator's VLNV through Bridge:
        assert driver.write_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, 0x23020001) == 0
        assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, byref(reg)) == 0 # Read TID
        assert reg.value == 0xFF
        assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, byref(reg)) == 0
        assert reg.value == 0x1003000b
        assert driver.read_register_callback(driver._drm_ctrl_base_addr + RegBridgeStream, byref(reg)) == 0
        assert reg.value == (0x00010001 + 0x10000 * i)


@pytest.mark.awsxrt
def test_vitis_1activator_som_125(accelize_drm, async_handler, log_file_factory):
    """
    Test drm controller bridge used for SoM projects: single clock
    """
    # Program board with design
    design_name = 'vitis_1activator_som_125'
    run_test_on_design(accelize_drm, design_name, False)


@pytest.mark.awsxrt
def test_vitis_1activator_som_200_125(accelize_drm, async_handler, log_file_factory):
    """
    Test drm controller bridge used for SoM projects: dual clock
    """
    # Program board with design
    design_name = 'vitis_1activator_som_200_125'
    run_test_on_design(accelize_drm, design_name, True)


@pytest.mark.awsxrt
def test_vitis_2activator_som_vhdl_100_125(accelize_drm, async_handler, log_file_factory):
    """
    Test drm controller bridge used for SoM projects: dual clock
    """
    # Program board with design
    design_name = 'vitis_2activator_som_vhdl_100_125'
    run_test_on_design(accelize_drm, design_name, True, 2)

