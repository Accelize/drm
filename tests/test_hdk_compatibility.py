# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
import gc
from glob import glob
from os import remove, getpid
from os.path import getsize, isfile, dirname, join, realpath
from re import match, search, finditer, MULTILINE, IGNORECASE
from time import sleep, time
from json import loads
from datetime import datetime, timedelta
from tests.conftest import wait_func_true


@pytest.mark.minimum
def test_backward_compatibility(accelize_drm, conf_json, cred_json, async_handler):
    from itertools import groupby
    """Test API is not compatible with DRM HDK inferior major number"""
    refdesign = accelize_drm.pytest_ref_designs
    hdk_version = accelize_drm.pytest_hdk_version
    if hdk_version is None:
        pytest.skip("FPGA image is not corresponding to a known HDK version")

    current_major = int(match(r'^(\d+)\.', hdk_version).group(1))
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    drm_manager = None

    try:
        refdesignByMajor = ((int(match(r'^(\d+)\.', x).group(1)), x) for x in refdesign.hdk_versions)

        for major, versions in groupby(refdesignByMajor, lambda x: x[0]):
            if major >= current_major:
                continue

            hdk = sorted((e[1] for e in versions))[0]
            # Program FPGA with older HDK
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)
            # Test compatibility issue
            with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
                async_cb.reset()
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
            if search(r'This DRM Library version \S+ is not compatible with the DRM HDK version', str(excinfo.value)):
                hit =True
            assert hit
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
            async_cb.assert_NoError()
            print('Test compatibility with %s: PASS' % hdk)

    finally:
        if drm_manager:
            del drm_manager
        gc.collect()
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)

'''
@pytest.mark.minimum
def test_hdk_uncompatibility(accelize_drm, conf_json, cred_json, async_handler):
    from itertools import groupby
    """Test API is not compatible with DRM HDK < 3.0"""
    refdesign = accelize_drm.pytest_ref_designs
    hdk_version = accelize_drm.pytest_hdk_version
    if hdk_version is None:
        pytest.skip("FPGA image is not corresponding to a known HDK version")

    current_major = int(match(r'^(\d+)\.', hdk_version).group(1))
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    drm_manager = None

    try:
        refdesignByMajor = ((int(match(r'^(\d+)\.', x).group(1)), x) for x in refdesign.hdk_versions)

        for major, versions in groupby(refdesignByMajor, lambda x: x[0]):
            if major >= current_major:
                continue

            hdk = sorted((e[1] for e in versions))[0]
            # Program FPGA with older HDK
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)
            # Test compatibility issue
            with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
                async_cb.reset()
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
            if search(r'This DRM Library version \S+ is not compatible with the DRM HDK version', str(excinfo.value)):
                hit =True
            assert hit
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
            async_cb.assert_NoError()
            print('Test compatibility with %s: PASS' % hdk)

    finally:
        if drm_manager:
            del drm_manager
        gc.collect()
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)
'''
