# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from itertools import groupby
from re import match, search, IGNORECASE


def test_hdk_stability_on_programming(accelize_drm, conf_json, cred_json, async_handler):
    """Test reprogramming multiple times does not produce any issue"""
    driver = accelize_drm.pytest_fpga_driver[0]
    image_id = driver.fpga_image
    async_cb = async_handler.create()
    async_cb.reset()
    nb_reset = 5
    for i in range(nb_reset):
        # Program FPGA with lastest HDK per major number
        driver.program_fpga(image_id)

        # Test no compatibility issue
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert not drm_manager.get('license_status')
            drm_manager.activate()
            assert drm_manager.get('license_status')
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
        async_cb.assert_NoError()


@pytest.mark.minimum
def test_uncompatibilities(accelize_drm, conf_json, cred_json, async_handler):
    """Test API is not compatible with DRM HDK inferior major number"""
    hdk_version = accelize_drm.pytest_hdk_version
    if hdk_version is None:
        pytest.skip("No HDK version found with FPGA image")

    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    async_cb.reset()

    # First instanciate an object to get the HDK compatbility version
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        try:
            HDK_Limit = float(drm_manager.get('hdk_compatibility'))
        except:
            HDK_Limit = 3.1

    # Then test all HDK versions that are not compatible
    refdesign = accelize_drm.pytest_ref_designs
    refdesignByMajor = ((float(match(r'^(\d+.\d+)', x).group(1)), x) for x in refdesign.hdk_versions)
    current_num = float(match(r'^(\d+.\d+)', hdk_version).group(1))
    tested = False
    try:
        for num, versions in groupby(refdesignByMajor, lambda x: x[0]):
            if HDK_Limit <= num and num <= current_num:
                print('Test uncompatible HDK: HDK version %s is in the compatiblity range[%s : %s]: skip version' % (num, HDK_Limit, current_num))
                continue
            tested = True

            # Program FPGA with older HDK
            hdks = [e[1] for e in versions]
            hdk = sorted(hdks, key=lambda x: list(map(int, x.split('.'))))[0]
            print('Testing HDK version %s is not compatible ...' % hdk)
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)

            # Test compatibility issue
            with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
                accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                )
            hit = False
            if 'Unable to find DRM Controller registers' in str(excinfo.value):
                hit = True
            if search(r'This DRM Library version \S+ is not compatible with the DRM HDK version', str(excinfo.value), IGNORECASE):
                hit = True
            assert hit
        if not tested:
            pytest.skip("Could not find a refdesign in the testsuite to test the uncompatible HDKs")
    finally:
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)


@pytest.mark.minimum
def test_compatibilities(accelize_drm, conf_json, cred_json, async_handler):
    """Test API is compatible with DRM HDK with the same major number"""
    hdk_version = accelize_drm.pytest_hdk_version
    if hdk_version is None:
        pytest.skip("FPGA image is not corresponding to a known HDK version")

    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    async_cb.reset()

    # First instanciate an object to get the HDK compatbility version
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        try:
            HDK_Limit = float(drm_manager.get('hdk_compatibility'))
        except:
            HDK_Limit = 3.1

    # Then test all HDK versions that are compatible
    refdesign = accelize_drm.pytest_ref_designs
    refdesignByMajor = ((float(match(r'^(\d+.\d+)', x).group(1)), x) for x in refdesign.hdk_versions)
    current_num = float(match(r'^(\d+.\d+)', hdk_version).group(1))
    tested = False
    try:
        for num, versions in groupby(refdesignByMajor, lambda x: x[0]):
            if num < HDK_Limit or num > current_num:
                print('Test compatible HDK: HDK version %s is not in the range ]%s : %s[: skip version' % (num, HDK_Limit, current_num))
                continue
            tested = True

            # Program FPGA with lastest HDK per major number
            hdks = [e[1] for e in versions]
            if hdk_version in hdks:
                hdks.remove(hdk_version)
            if len(hdks) == 0:
                # The current version is the only version with this major number
                continue
            hdk = sorted(hdks, key=lambda x: list(map(int, x.split('.'))))[-1]
            print('Testing HDK version %s is compatible ...' % hdk)
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)

            # Test no compatibility issue
            with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
                assert not drm_manager.get('license_status')
                drm_manager.activate()
                assert drm_manager.get('license_status')
                drm_manager.deactivate()
                assert not drm_manager.get('license_status')
            async_cb.assert_NoError()
        assert tested
    finally:
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)


def test_ctrl_version_match_mailbox_version(accelize_drm, conf_json, cred_json, async_handler):
    """Verify the version in the controller register match the version in the read-only mailbox"""
    if accelize_drm.is_ctrl_sw:
        pytest.skip("DRM Bridge version differs from the version in the read-only mailbox")

    driver = accelize_drm.pytest_fpga_driver[0]
    image_id = driver.fpga_image
    async_cb = async_handler.create()
    async_cb.reset()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        ctrl_version_str = drm_manager.get('controller_version')
        ctrl_rom = drm_manager.get('controller_rom')
    ctrl_version_match = match(r'(\d+)\.(\d+)\.(\d+)', ctrl_version_str)
    rom_version_match = match(r'(\d+)\.(\d+)\.(\d+)\.\d+', ctrl_rom['extra']['lgdn_full_version'])
    ctrl_version = ''.join(map(lambda x: '%02X' % int(x), rom_version_match.groups()))
    rom_version = ''.join(map(lambda x: '%02X' % int(x), rom_version_match.groups()))
    assert rom_version == ctrl_version
    async_cb.assert_NoError()

