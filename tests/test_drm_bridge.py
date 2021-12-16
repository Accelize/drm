# -*- coding: utf-8 -*-
"""
Test all other vitis reference designs
"""
import pytest
from ctypes import c_uint, byref
from json import loads
from random import randint


REG_BRIDGE_VERSION       =  0x0
REG_BRIDGE_STREAM        =  0x4
REG_BRIDGE_MAILBOX_SIZE  =  0x8
REG_BRIDGE_MAILBOX_RO    =  0xC


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


def run_test_on_design(accelize_drm, design_name ):

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
    assert driver.read_register_callback(driver._drm_ctrl_base_addr + REG_BRIDGE_VERSION, byref(reg)) == 0
    assert reg.value == 0x01000000

    # Test controller bridge mailbox size
    assert driver.read_register_callback(driver._drm_ctrl_base_addr + REG_BRIDGE_MAILBOX_SIZE, byref(reg)) == 0
    mailbox_ro_size = reg.value >> 16
    mailbox_rw_size = reg.value & 0xFFFF
    assert mailbox_ro_size > 0x10
    assert mailbox_rw_size > 0

    # Test controller bridge mailbox read-only content
    hex_str = ''
    for i in range(mailbox_ro_size):
        assert driver.read_register_callback(driver._drm_ctrl_base_addr + REG_BRIDGE_MAILBOX_RO + 4*i, byref(reg)) == 0
        hex_str += '%08X' % reg.value
    text_str = stringify(hex_str)
    text_json = loads(text_str)
    assert text_json['fpga_family'] == 'random_id'
    assert text_json['fpga_vendor'] == 'xilinx'
    assert text_json['product_id']['vendor'] == 'accelize.com'
    assert text_json['product_id']['library'] == 'refdesign'
    assert text_json['product_id']['name'] == 'drm_1activator'
    assert text_json['extra']['csp'] == 'aws-f1'
    assert 'dualclk' in text_json['extra'], "text_json=%s" % str(text_json)
    assert text_json['drm_software'], "text_json=%s" % str(text_json)

    # Test controller bridge mailbox read-write content
    ref_list = []
    for i in range(mailbox_rw_size):
        ref = randint(1,0xFFFFFFFF)
        assert driver.write_register_callback(driver._drm_ctrl_base_addr + REG_BRIDGE_MAILBOX_RO + 4*mailbox_ro_size + 4*i, ref) == 0
        ref_list.append(ref)
    for i in range(mailbox_rw_size):
        assert driver.read_register_callback(driver._drm_ctrl_base_addr + REG_BRIDGE_MAILBOX_RO + 4*mailbox_ro_size + 4*i, byref(reg)) == 0
        assert reg.value == ref_list[i]

    # Test read of activator VLNV from bridge
    assert driver.write_register_callback(driver._drm_ctrl_base_addr + REG_BRIDGE_STREAM, 0x23020001) == 0
    assert driver.read_register_callback(driver._drm_ctrl_base_addr + REG_BRIDGE_STREAM, byref(reg)) == 0
    assert reg.value == 0x1003000B
    assert driver.read_register_callback(driver._drm_ctrl_base_addr + REG_BRIDGE_STREAM, byref(reg)) == 0
    assert reg.value == 0x00010001


@pytest.mark.awsxrt
def test_vitis_1activator_som_125(accelize_drm, async_handler, log_file_factory):
    """
    Test drm controller bridge used for SoM projects: single clock
    """
    # Program board with design
    design_name = 'vitis_1activator_som_125'
    run_test_on_design(accelize_drm, design_name)


@pytest.mark.awsxrt
def test_vitis_1activator_som_200_125(accelize_drm, async_handler, log_file_factory):
    """
    Test drm controller bridge used for SoM projects: dual clock
    """
    # Program board with design
    design_name = 'vitis_1activator_som_200_125'
    run_test_on_design(accelize_drm, design_name)
