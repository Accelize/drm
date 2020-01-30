# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
from time import sleep
from random import randint
from datetime import datetime, timedelta
from re import search
import pytest

from multiprocessing import Process


@pytest.mark.skip(reason='Under development')
def test_session_id(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test an error is return if a wrong session id is provided
    """

    def action():
        print('ACTION!!!!!!!!!!!!')
        return 200, 'GOT SERVER'

    srv = fake_server()
    srv.add_endpoint('/', handler=action)
    srv.run()

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
    conf_json['licensing']['url'] = "http://127.0.0.1:5000"
    conf_json.save()

    server = Process(target=fksrv.app.run)
    print('Starting server')
    server.start()
    try:
        print('Creating drm manager')
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        try:
            print('Activating')
            drm_manager.activate()
            print('sleeping')
            sleep(1)
        finally:
            print('Deactivating')
            drm_manager.deactivate()
    finally:
        print('terminating server')
        server.terminate()
        server.join()
