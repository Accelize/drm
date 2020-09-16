# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from os import remove
from itertools import groupby
from re import match, search, IGNORECASE
from tests.conftest import whoami


def test_hdk_stability_on_programming(accelize_drm, conf_json, cred_json, async_handler):
    """Test reprogramming multiple times does not produce any issue"""
    driver = accelize_drm.pytest_fpga_driver[0]
    image_id = driver.fpga_image
    async_cb = async_handler.create()
    async_cb.reset()
    drm_manager = None

    conf_json.reset()
    logpath = accelize_drm.create_log_path(whoami())
    conf_json['settings']['log_file_verbosity'] = accelize_drm.create_log_level(0)
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json['settings']['log_file_append'] = True
    conf_json.save()

    nb_reset = 10
    for i in range(nb_reset):
        # Program FPGA with lastest HDK per major number
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
        try:
            drm_manager.activate()
            assert drm_manager.get('license_status')
        finally:
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
        del drm_manager
        async_cb.assert_NoError()
    remove(logpath)


@pytest.mark.minimum
def test_uncompatibilities(accelize_drm, conf_json, cred_json, async_handler):
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
        # First instanciate an object to get the HDK compatbility version
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        try:
            HDK_Limit = float(drm_manager.get('hdk_compatibility'))
        except:
            HDK_Limit = 3.1

        # Then test all HDK versions that are not compatible
        current_num = float(match(r'^(\d+.\d+)', hdk_version).group(1))
        refdesignByMajor = ((float(match(r'^(\d+.\d+)', x).group(1)), x) for x in refdesign.hdk_versions)

        tested = False

        for num, versions in groupby(refdesignByMajor, lambda x: x[0]):
            if HDK_Limit <= num and num <= current_num:
                print('Test uncompatible HDK: HDK version %s is in the compatiblity range[%s : %s]: skip version' % (num, HDK_Limit, current_num))
                continue
            tested = True

            print('Testing HDK version %s is not compatible ...' % num)
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
            hit = False
            if 'Unable to find DRM Controller registers' in str(excinfo.value):
                hit =True
            if search(r'This DRM Library version \S+ is not compatible with the DRM HDK version', str(excinfo.value), IGNORECASE):
                hit =True
            assert hit
        assert tested

    finally:
        if drm_manager:
            del drm_manager
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)


@pytest.mark.minimum
def test_compatibilities(accelize_drm, conf_json, cred_json, async_handler):
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
        # First instanciate an object to get the HDK compatbility version
        try:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
            HDK_Limit = float(drm_manager.get('hdk_compatibility'))
        except:
            HDK_Limit = 3.1

        # Then test all HDK versions that are compatible
        current_num = float(match(r'^(\d+.\d+)', hdk_version).group(1))
        refdesignByMajor = ((float(match(r'^(\d+.\d+)', x).group(1)), x) for x in refdesign.hdk_versions)
        tested = False

        for num, versions in groupby(refdesignByMajor, lambda x: x[0]):
            if num < HDK_Limit or num > current_num:
                print('Test compatible HDK: HDK version %s is not in the range ]%s : %s[: skip version' % (num, HDK_Limit, current_num))
                continue
            tested = True

            print('Testing HDK version %s is compatible ...' % num)
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
        assert tested
    finally:
        if drm_manager:
            del drm_manager
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)
