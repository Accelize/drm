# -*- coding: utf-8 -*-
"""
Test all other vitis reference designs
"""
import pytest
from datetime import datetime
from re import search, match, IGNORECASE

import tests.conftest as conftest


def run_test_on_design(accelize_drm, design_name, conf_json, cred_json, async_handler,
                       log_file_factory, axiclk_freq_ref, drmclk_freq_ref):

    # Program board with design
    ref_designs = accelize_drm.pytest_ref_designs
    try:
        fpga_image = ref_designs.get_image_id(design_name)
    except:
        pytest.skip(f"Could not find refesign name '{design_name}' for driver '{accelize_drm.pytest_fpga_driver_name}'")
    driver = accelize_drm.pytest_fpga_driver[0]
    driver.program_fpga(fpga_image)
    accelize_drm.scanActivators()
    pytest_hdk_version =  match(r'(.+)\.\d+$', accelize_drm.pytest_hdk_version).group(1)

    # Run test
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json['drm']['frequency_mhz'] = drmclk_freq_ref
    conf_json.save()
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert pytest_hdk_version == drm_manager.get('controller_version')
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        assert drm_manager.get('session_id') == ''
        activators.autotest(is_activated=False)
        activators.generate_coin(5)
        activators.check_coin()
        # Start session
        drm_manager.activate()
        start = datetime.now()
        assert sum(drm_manager.get('metered_data')) == 0
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        activators.autotest(is_activated=True)
        lic_duration = drm_manager.get('license_duration')
        activators.generate_coin()
        activators.check_coin(drm_manager.get('metered_data'))
        # Wait until 2 licenses are provisioned
        conftest.wait_func_true(lambda: drm_manager.get('num_license_loaded') == 2, lic_duration)
        # Pause session
        drm_manager.deactivate(True)
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        activators.autotest(is_activated=True)
        # Wait right before license expiration
        conftest.wait_deadline(start, 2*lic_duration-3)
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        activators.autotest(is_activated=True)
        # Wait expiration
        conftest.wait_deadline(start, 2*lic_duration+2)
        assert drm_manager.get('session_status')
        assert drm_manager.get('session_id') == session_id
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        activators.generate_coin()
        activators.check_coin(drm_manager.get('metered_data'))
        # Resume session
        drm_manager.activate(True)
        assert drm_manager.get('session_status')
        assert drm_manager.get('session_id') != session_id
        assert drm_manager.get('license_status')
        activators.reset_coin()
        activators.autotest(is_activated=True)
        activators.generate_coin()
        activators.check_coin(drm_manager.get('metered_data'))
        # Stop session
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert drm_manager.get('session_id') == ''
    # Check result
    log_content = logfile.read()
    # Check API calls
    assert search(r"Calling Impl public constructor", log_content, IGNORECASE)
    assert search(r"Calling 'activate' with 'resume_session_request'=false", log_content, IGNORECASE)
    assert search(r"Calling 'deactivate' with 'pause_session_request'=true", log_content, IGNORECASE)
    assert search(r"Calling 'activate' with 'resume_session_request'=true", log_content, IGNORECASE)
    assert search(r"Calling 'deactivate' with 'pause_session_request'=false", log_content, IGNORECASE)
    # Check DRM Controller frequencies
    drmclk_match = search(r'Frequency detection of drm_aclk counter after .+ => estimated frequency = (\d+) MHz', log_content)
    drmclk_freq_measure = int(drmclk_match.group(1))
    assert drmclk_freq_ref*0.9 < drmclk_freq_measure < drmclk_freq_ref*1.1
    axiclk_match = search(r'Frequency detection of s_axi_aclk counter after .+ => estimated frequency = (\d+) MHz', log_content)
    axiclk_freq_measure = int(axiclk_match.group(1))
    assert axiclk_freq_ref*0.9 < axiclk_freq_measure < axiclk_freq_ref*1.1
    # Return logfile handler for more specific verification
    logfile.remove()
    return log_content


@pytest.mark.awsf1
def test_1activator_125(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test a RTL vivado configuration: 1 single clock kernels with Verilog source
    """
    # Run test
    design_name = '1activator_axi4_125'
    axiclk_freq_ref = 125
    drmclk_freq_ref = 125
    log_content = run_test_on_design(accelize_drm, design_name, conf_json, cred_json, async_handler,
                                    log_file_factory, axiclk_freq_ref, drmclk_freq_ref)


@pytest.mark.awsf1
def test_2activator_125(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test a RTL vivado configuration: single clock kernels with Verilog source
    """
    # Run test
    design_name = '2activator_125'
    axiclk_freq_ref = 125
    drmclk_freq_ref = 125
    log_content = run_test_on_design(accelize_drm, design_name, conf_json, cred_json, async_handler,
                                    log_file_factory, axiclk_freq_ref, drmclk_freq_ref)


@pytest.mark.awsf1
def test_2activator_15_125(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test a RTL vivado configuration: dual clock kernels with Verilog source
    """
    # Run test
    design_name = '2activator_15_125'
    axiclk_freq_ref = 15
    drmclk_freq_ref = 125
    log_content = run_test_on_design(accelize_drm, design_name, conf_json, cred_json, async_handler,
                                    log_file_factory, axiclk_freq_ref, drmclk_freq_ref)


@pytest.mark.awsf1
def test_2activator_vhdl_125(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test a RTL vivado configuration: single clock kernels with VHDL source
    """
    # Run test
    design_name = '2activator_vhdl_125'
    axiclk_freq_ref = 125
    drmclk_freq_ref = 125
    log_content = run_test_on_design(accelize_drm, design_name, conf_json, cred_json, async_handler,
                                    log_file_factory, axiclk_freq_ref, drmclk_freq_ref)


@pytest.mark.skip
@pytest.mark.awsf1
def test_2activator_vhdl_15_225(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test a RTL vivado configuration: dual clock kernels with VHDL source
    """
    # Run test
    design_name = '2activator_vhdl_15_225'
    axiclk_freq_ref = 15
    drmclk_freq_ref = 225
    log_content = run_test_on_design(accelize_drm, design_name, conf_json, cred_json, async_handler,
                                    log_file_factory, axiclk_freq_ref, drmclk_freq_ref)


@pytest.mark.awsf1
def test_2activator_250_75_swap_activator(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test a RTL vivado configuration: dual clock kernels with activators inverted on LGDN bus
    """
    # Run test
    design_name = '2activator_axi4_swap_activator_250_125'
    axiclk_freq_ref = 250
    drmclk_freq_ref = 125
    log_content = run_test_on_design(accelize_drm, design_name, conf_json, cred_json, async_handler,
                                    log_file_factory, axiclk_freq_ref, drmclk_freq_ref)


@pytest.mark.skip(reason='To be manually executed: only used to check the behavior of the DRM Controller IP when 1 activator is missing out of 2')
@pytest.mark.awsf1
def test_2activator_missing_1activator(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test a RTL vivado configuration: dual clock kernels with one missing activator
    Verify that is works as expected
    """
    # Run test
    design_name = '2activator_missing_1activator'
    axiclk_freq_ref = 15
    drmclk_freq_ref = 125
    log_content = run_test_on_design(accelize_drm, design_name, conf_json, cred_json, async_handler,
                                    log_file_factory, axiclk_freq_ref, drmclk_freq_ref)
