# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
import pytest
from datetime import datetime
from os.path import join, dirname, realpath
import tests.conftest as conftest
from tests.fpga_drivers import get_driver

SCRIPT_DIR = dirname(realpath(__file__))


@pytest.mark.skip
def test_vitis_2activator_vhdl_250_125(pytestconfig, conf_json, cred_json, async_handler):
    """
    Test one vitis configuration: dual clock kernel with different frequencies
    """
    import accelize_drm as _accelize_drm
    driver_name = 'xilinx_xrt'
    design_name = 'vitis_2activator_vhdl_250_125'

    slot_id = pytestconfig.getoption("fpga_slot_id")
    ref_designs = conftest.RefDesign(join(SCRIPT_DIR, 'refdesigns', driver_name))
    fpga_image = ref_designs.get_image_id(design_name)
    drm_ctrl_base_addr = pytestconfig.getoption("drm_controller_base_address")
    no_clear_fpga = pytestconfig.getoption("no_clear_fpga")

    # Create driver and program FPGA
    fpga_driver_cls = get_driver(driver_name)
    driver = fpga_driver_cls(
                    fpga_slot_id = slot_id,
                    fpga_image = fpga_image,
                    drm_ctrl_base_addr = drm_ctrl_base_addr,
                    no_clear_fpga = no_clear_fpga
                )

    # Get activators
    base_addr = pytestconfig.getoption("activator_base_address")
    activators = conftest.findActivators(driver, base_addr)
    activators.reset_coin()
    activators.autotest()

    # Run test
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    drm_manager = _accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        assert drm_manager.get('session_id') == ''
        activators.autotest(is_activated=False)
        # Start session
        drm_manager.activate()
        start = datetime.now()
        assert drm_manager.get('metered_data') == 0
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        activators.autotest(is_activated=True)
        lic_duration = drm_manager.get('license_duration')
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        # Wait until 2 licenses are provisioned
        wait_func_true(lambda: drm_manager.get('num_license_loaded') == 2, lic_duration)
        # Pause session
        drm_manager.deactivate(True)
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        activators.autotest(is_activated=True)
        # Wait right before license expiration
        wait_deadline(start, 2*lic_duration-3)
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        activators.autotest(is_activated=True)
        # Wait expiration
        wait_deadline(start, 2*lic_duration+2)
        assert drm_manager.get('session_status')
        assert drm_manager.get('session_id') == session_id
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        # Resume session
        drm_manager.activate(True)
        assert drm_manager.get('session_status')
        assert drm_manager.get('session_id') != session_id
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        # Stop session
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert drm_manager.get('session_id') == ''
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()

