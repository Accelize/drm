# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from re import match, search, IGNORECASE


HDK_COMPATIBLITY_LIMIT = 3.1


@pytest.mark.minimum
def test_uncompatibilities(accelize_drm, conf_json, cred_json, async_handler):
    from itertools import groupby
    """Test API is not compatible with DRM HDK inferior major number"""
    refdesign = accelize_drm.pytest_ref_designs
    hdk_version = accelize_drm.pytest_hdk_version
    if hdk_version is None:
        pytest.skip("No HDK version found with FPGA image")

    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    async_cb.reset()
    drm_manager = None

    try:
        current_num = float(match(r'^(\d+.\d+)', hdk_version).group(1))
        refdesignByMajor = ((float(match(r'^(\d+.\d+)', x).group(1)), x) for x in refdesign.hdk_versions)

        for num, versions in groupby(refdesignByMajor, lambda x: x[0]):
            if HDK_COMPATIBLITY_LIMIT <= num and num <= current_num:
                continue

            # Program FPGA with older HDK
            hdk = sorted((e[1] for e in versions))[0]
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)

            # Test compatibility issue
            with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
                drm_manager = accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                )
            #hit = False
            #if 'Unable to find DRM Controller registers' in str(excinfo.value):
            #    hit =True
            #if search(r'This DRM Library version \S+ is not compatible with the DRM HDK version', str(excinfo.value)):
            #    hit =True
            #assert hit
            assert search(r'This DRM Library version \S+ is not compatible with the DRM HDK version', str(excinfo.value), IGNORECASE)
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
            async_cb.assert_NoError()

    finally:
        if drm_manager:
            del drm_manager
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)


@pytest.mark.minimum
def test_hdk_compatibility(accelize_drm, conf_json, cred_json, async_handler):
    from itertools import groupby
    """Test API is compatible with DRM HDK with the same major number"""
    refdesign = accelize_drm.pytest_ref_designs
    hdk_version = accelize_drm.pytest_hdk_version
    if hdk_version is None:
        pytest.skip("FPGA image is not corresponding to a known HDK version")

    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    async_cb.reset()
    drm_manager = None

    try:
        current_num = float(match(r'^(\d+.\d+)', hdk_version).group(1))
        refdesignByMajor = ((float(match(r'^(\d+.\d+)', x).group(1)), x) for x in refdesign.hdk_versions)

        for num, versions in groupby(refdesignByMajor, lambda x: x[0]):
            if num < HDK_COMPATIBLITY_LIMIT or num > current_num:
                continue

            # Program FPGA with lastest HDK per major number
            hdk = sorted((e[1] for e in versions))[-1]
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)

            # Test no compatibility issue
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
            assert not drm_manager.get('license_status')
            drm_manager.activate()
            assert drm_manager.get('license_status')
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
            async_cb.assert_NoError()
    finally:
        if drm_manager:
            del drm_manager
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)
