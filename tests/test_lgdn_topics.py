# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randint
from datetime import datetime

from tests.conftest import wait_deadline


@pytest.mark.lgdn
def test_topic1_corrupted_metering(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test no error occurs in normal start/stop metering mode during a long period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    nb_run = 100
    nb_pause_resume_max = 100
    for r in range(nb_run):
        print('Run #%d' % r)
        try:
            activators[0].reset_coin()
            assert not drm_manager.get('session_status')
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            async_cb.assert_NoError()
            drm_manager.activate()
            start = datetime.now()
            assert drm_manager.get('metered_data') == 0
            assert drm_manager.get('session_status')
            assert drm_manager.get('license_status')
            session_id = drm_manager.get('session_id')
            assert len(session_id) > 0
            lic_duration = drm_manager.get('license_duration')
            activators.autotest(is_activated=True)
            for i in range(nb_pause_resume_max):
                print('Pause #%d' % i)
                try:
                    new_coins = randint(1, 100)
                    activators[0].generate_coin(new_coins)
                    data = drm_manager.get('metered_data')
                    try:
                        activators[0].check_coin(data)
                    except AssertionError:
                        print("ERROR detected!!!!!!!!")
                        print("1st read gives:", data)
                        print("Waiting 5s ...")
                        sleep(5)
                        print("... and double check the metering")
                        data = drm_manager.get('metered_data')
                        print("2nd read gives:", data)
                        activators[0].check_coin(drm_manager.get('metered_data'))
                    drm_manager.deactivate(True)
                    async_cb.assert_NoError()
                    assert drm_manager.get('session_status')
                    assert drm_manager.get('license_status')
                    assert drm_manager.get('session_id') == session_id
                    # Wait randomly at the limit of the expiration
                    random_wait = lic_duration*2
                    wait_deadline(start, random_wait)
                    drm_manager.activate(True)
                    start = datetime.now()
                except:
                    raise
            drm_manager.deactivate()
            assert not drm_manager.get('session_status')
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            assert drm_manager.get('session_id') != session_id
            async_cb.assert_NoError()
        finally:
            drm_manager.deactivate()
