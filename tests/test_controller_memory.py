# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from re import search


@pytest.mark.minimum
def test_wrong_drm_controller_address(accelize_drm, conf_json, cred_json, async_handler):
    """Test when a wrong DRM Controller offset is given"""
    async_cb = async_handler.create()
    async_cb.reset()
    driver = accelize_drm.pytest_fpga_driver[0]
    ctrl_base_addr_backup = driver._drm_ctrl_base_addr
    driver._drm_ctrl_base_addr += 0x10000
    try:
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert 'Unable to find DRM Controller registers.' in str(excinfo.value)
        assert 'Please verify' in str(excinfo.value)
    finally:
        driver._drm_ctrl_base_addr = ctrl_base_addr_backup


def test_mailbox_write_overflow(accelize_drm, conf_json, cred_json, async_handler):
    from random import sample

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test with a null crendential file
    async_cb.reset()
    cred_json.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    mb_size = drm_manager.get('mailbox_size')
    assert mb_size > 0

    mb_data = sample(range(0xFFFFFFFF), mb_size + 1)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(mailbox_data=mb_data)
    assert 'Trying to write out of Mailbox memory space' in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()


def test_mailbox_type_error(accelize_drm, conf_json, cred_json, async_handler):
    from random import sample

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test with a null crendential file
    async_cb.reset()
    cred_json.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(mailbox_data='this is bad type')
    assert 'Value must be an array of integers' in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()


def test_configuration_file_empty_and_corrupted_product_id(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when an incorrect product ID is requested to Web Server"""

    refdesign = accelize_drm.pytest_ref_designs
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    try:
        # Test Web Service when an empty product ID is provided
        empty_fpga_image = refdesign.get_image_id('empty_product_id')
        if empty_fpga_image is None:
            pytest.skip("No FPGA image found for 'empty_product_id'")
        driver.program_fpga(empty_fpga_image)
        async_cb.reset()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('product_info') is None
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'Metering Web Service error 400' in str(excinfo.value)
        assert 'DRM WS request failed' in str(excinfo.value)
        assert search(r'\\"Unknown Product ID\\" for ', str(excinfo.value)) is not None
        assert 'Product ID from license request is not set' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
        print('Test Web Service when an empty product ID is provided: PASS')

        # Test when a misformatted product ID is provided
        bad_fpga_image = refdesign.get_image_id('bad_product_id')
        if bad_fpga_image is None:
            pytest.skip("No FPGA image found for 'bad_product_id'")
        driver.program_fpga(bad_fpga_image)
        async_cb.reset()
        with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert 'Failed to parse Read-Only Mailbox in DRM Controller:' in str(excinfo.value)
        assert search(r'Cannot parse JSON string ', str(excinfo.value))
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadFormat.error_code
        async_cb.assert_NoError()
        print('Test Web Service when a misformatted product ID is provided: PASS')

    finally:
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)


@pytest.mark.skip(reason='Not supported')
def test_2_drm_manager_concurrently(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when 2 DrmManager instances are used."""

    driver = accelize_drm.pytest_fpga_driver[0]

    async_cb1 = async_handler.create()
    async_cb2 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_02')

    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb1.callback
    )

    with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
        drm_manager2 = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb2.callback
        )
    assert 'Another instance of the DRM Manager is currently owning the HW' in str(excinfo.value)


@pytest.mark.hwtst
def test_drm_manager_bist(accelize_drm, conf_json, cred_json, async_handler):
    """Test register access BIST"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test read callback error
    def my_wrong_read_callback(register_offset, returned_data):
        addr = register_offset
        if register_offset > 0 and register_offset <= 0x40:
            addr += 0x4
        return driver.read_register_callback(addr, returned_data, driver)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            my_wrong_read_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'DRM Communication Self-Test 2 failed' in str(excinfo.value)
    assert 'Please verify' in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()

    # Test write callback error
    def my_wrong_write_callback(register_offset, data_to_write):
        return driver.write_register_callback(register_offset*2, data_to_write, driver)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            my_wrong_write_callback,
            async_cb.callback
        )
    assert 'DRM Communication Self-Test 2 failed' in str(excinfo.value)
    assert 'Please verify' in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()
