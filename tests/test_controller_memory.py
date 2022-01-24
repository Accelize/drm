# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from re import search, IGNORECASE


def test_wrong_drm_controller_address(accelize_drm, conf_json, cred_json, async_handler,
                log_file_factory):
    """Test when a wrong DRM Controller offset is given"""
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves callbacks modification: skipped on SoM target (no callback provided for SoM)")

    async_cb = async_handler.create()
    async_cb.reset()
    driver = accelize_drm.pytest_fpga_driver[0]
    ctrl_base_addr_backup = driver._drm_ctrl_base_addr
    driver._drm_ctrl_base_addr += 0x10000
    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    try:
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert 'Unable to find DRM Controller registers.' in str(excinfo.value)
        assert 'Please verify' in str(excinfo.value)
    finally:
        driver._drm_ctrl_base_addr = ctrl_base_addr_backup


def test_mailbox_write_overflow(accelize_drm, conf_json, cred_json, async_handler,
                log_file_factory):
    from random import sample

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test with a null crendential file
    async_cb.reset()
    cred_json.reset()
    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        mb_size = drm_manager.get('mailbox_size')
        assert mb_size > 0
        mb_data = sample(range(0xFFFFFFFF), mb_size + 1)
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(mailbox_data=mb_data)
        assert 'Trying to write out of Mailbox memory space' in str(excinfo.value)
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'Trying to write out of Mailbox memory space')
        async_cb.reset()


def test_mailbox_type_error(accelize_drm, conf_json, cred_json, async_handler,
                log_file_factory):
    from random import sample

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test with a null crendential file
    async_cb.reset()
    cred_json.reset()
    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(mailbox_data='this is bad type')
        assert 'Value must be an array of integers' in str(excinfo.value)
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'Value must be an array of integers')
    async_cb.reset()


def test_empty_product_id(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """Test error with a design having an empty Product ID"""
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test skipped on SoM target")

    refdesign = accelize_drm.pytest_ref_designs
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    empty_fpga_image = refdesign.get_image_id('empty_product_id')
    if empty_fpga_image is None:
        pytest.skip("No FPGA image found for 'empty_product_id'")

    try:
        driver.program_fpga(empty_fpga_image)
        async_cb.reset()
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
        assert search(r'UDID and Product ID cannot be both missing', str(excinfo.value), IGNORECASE)
        assert search(r'Could not find Product ID information in DRM Controller Memory', logfile.read(), IGNORECASE)
        async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'UDID and Product ID cannot be both missing')
        async_cb.reset()
    finally:
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)
    logfile.remove()


def test_malformed_product_id(accelize_drm, conf_json, cred_json, async_handler):
    """Test error with a design having a malformed Product ID"""
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test skipped on SoM target")

    refdesign = accelize_drm.pytest_ref_designs
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')
    bad_fpga_image = refdesign.get_image_id('bad_product_id')
    if bad_fpga_image is None:
        pytest.skip("No FPGA image found for 'bad_product_id'")

    try:
        driver.program_fpga(bad_fpga_image)
        async_cb.reset()
        with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert 'Failed to parse Read-Only Mailbox in DRM Controller:' in str(excinfo.value)
        assert search(r'Cannot parse JSON string ', str(excinfo.value))
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadFormat.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMBadFormat.error_code, 'Failed to parse Read-Only Mailbox in DRM Controller')
        async_cb.reset()
        print('Test Web Service when a misformatted product ID is provided: PASS')
    finally:
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)


@pytest.mark.skip(reason='Two concurrent objects on the same board is not supported')
def test_2_drm_manager_concurrently(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when 2 DrmManager instances are used."""

    driver = accelize_drm.pytest_fpga_driver[0]

    async_cb1 = async_handler.create()
    async_cb2 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_02')

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb1.callback
        ) as drm_manager1:

        with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
            drm_manager2 = accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb2.callback
            )
    assert 'Another instance of the DRM Manager is currently owning the HW' in str(excinfo.value)


@pytest.mark.hwtst
def test_drm_manager_bist(accelize_drm, conf_json, cred_json, async_handler):
    """Test register access BIST"""
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test skipped on SoM target")

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
            conf_json.path, cred_json.path,
            my_wrong_read_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'DRM Communication Self-Test 2 failed' in str(excinfo.value)
    assert 'Please verify' in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'DRM Communication Self-Test 2 failed')
    async_cb.reset()

    # Test write callback error
    def my_wrong_write_callback(register_offset, data_to_write):
        return driver.write_register_callback(register_offset*2, data_to_write, driver)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            my_wrong_write_callback,
            async_cb.callback
        )
    assert 'DRM Communication Self-Test 2 failed' in str(excinfo.value)
    assert 'Please verify' in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'DRM Communication Self-Test 2 failed')
    async_cb.reset()
