# -*- coding: utf-8 -*-
"""
Test derivated product feature
"""
import pytest
from time import sleep
from random import randrange, randint
from re import search, findall, MULTILINE, IGNORECASE
from dateutil import parser
from itertools import groupby
from flask import request as _request
from requests import get, post
from os import remove
from os.path import realpath, isfile

from tests.conftest import wait_func_true
from tests.proxy import get_context, set_context, get_proxy_error


def test_invalid_derivated_product_vendor(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test a invalid derivated product vendor returns an error
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    deriv_prod = drm_manager.get('derivated_product')
    new_deriv_prod = 'a' + deriv_prod
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(derivated_product=new_deriv_prod)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert "Invalid derivated product information: vendor mismatch" in str(excinfo.value)

    # Same test but using config file
    new_deriv_prod = 'b' + deriv_prod
    conf_json['derivated_product'] = new_deriv_prod
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert "Invalid derivated product information: vendor mismatch" in str(excinfo.value)


def test_invalid_derivated_product_library(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test a invalid derivated product library returns an error
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    deriv_prod = drm_manager.get('derivated_product')
    deriv_prod_list = deriv_prod.split('/')
    deriv_prod_list[1] += 'a'
    new_deriv_prod = '/'.join(deriv_prod_list)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(derivated_product=new_deriv_prod)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert "Invalid derivated product information: library mismatch" in str(excinfo.value)

    # Same test but using config file
    deriv_prod_list = deriv_prod.split('/')
    deriv_prod_list[1] += 'b'
    new_deriv_prod = '/'.join(deriv_prod_list)
    conf_json['derivated_product'] = new_deriv_prod
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert "Invalid derivated product information: library mismatch" in str(excinfo.value)


def test_invalid_derivated_product_name(accelize_drm, conf_json, cred_json,
                    async_handler):
    """
    Test a invalid derivated product name returns an error
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    deriv_prod = drm_manager.get('derivated_product')
    deriv_prod_list = deriv_prod.split('/')
    deriv_prod_list[2] = 'a' + deriv_prod_list[2]
    new_deriv_prod = '/'.join(deriv_prod_list)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(derivated_product=new_deriv_prod)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert "Invalid derivated product information: name mismatch" in str(excinfo.value)

    # Same test but using config file
    deriv_prod_list = deriv_prod.split('/')
    deriv_prod_list[2] = 'b' + deriv_prod_list[2]
    new_deriv_prod = '/'.join(deriv_prod_list)
    conf_json['derivated_product'] = new_deriv_prod
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert "Invalid derivated product information: name mismatch" in str(excinfo.value)


@pytest.mark.no_parallel
@pytest.mark.minimum
@pytest.mark.packages
def test_valid_derivated_product(accelize_drm, conf_json, cred_json,
                    async_handler, live_server, basic_log_file, request):
    """
    Test a valid derivated product behaves as expected.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json['settings'].update(basic_log_file.create(1))
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    deriv_prod = drm_manager.get('derivated_product')
    suffix = '_subproduct1'
    new_deriv_prod = deriv_prod + suffix
    drm_manager.set(derivated_product=new_deriv_prod)
    assert drm_manager.get('derivated_product') == new_deriv_prod

    # Set initial context on the live server
    context = {'product_suffix':suffix}
    set_context(context)
    assert get_context() == context

    try:
        drm_manager.activate()
        context = get_context()
        assert context['derivated_product'] == new_deriv_prod
    finally:
        drm_manager.deactivate()
    log_content = basic_log_file.read()
    assert search(f'Loaded new derivated product: {new_deriv_prod}', log_content, MULTILINE)
    assert get_proxy_error() is None
    async_cb.assert_NoError()

    # Same test but from config file
    suffix = '_subproduct2'
    new_deriv_prod = deriv_prod + suffix
    conf_json['derivated_product'] = new_deriv_prod
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('derivated_product') == new_deriv_prod

    # Set initial context on the live server
    context = {'product_suffix':suffix}
    set_context(context)
    assert get_context() == context

    try:
        drm_manager.activate()
        context = get_context()
        assert context['derivated_product'] == new_deriv_prod
    finally:
        drm_manager.deactivate()
    log_content = basic_log_file.read()
    assert search(f'Loaded new derivated product: {new_deriv_prod}', log_content, MULTILINE)
    assert get_proxy_error() is None
    async_cb.assert_NoError()
    basic_log_file.remove()


def test_derivated_product_during_running_session(accelize_drm, conf_json, cred_json,
                    async_handler):
    """
    Check that a derivated product cannot be modified when a session is already running
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    deriv_prod = drm_manager.get('derivated_product')
    new_deriv_prod = deriv_prod + '_subproduct'
    try:
        drm_manager.activate()
        # try to modify derivated product
        with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
            drm_manager.set(derivated_product=new_deriv_prod)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadUsage.error_code
        assert "Derivated product cannot be loaded if a session is still running" in str(excinfo.value)
    finally:
        drm_manager.deactivate()
        # new derivated product can now be loaded
        drm_manager.set(derivated_product=new_deriv_prod)
        assert drm_manager.get('derivated_product') == new_deriv_prod
    async_cb.assert_NoError()
