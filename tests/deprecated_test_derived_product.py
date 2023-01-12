# -*- coding: utf-8 -*-
"""
Test derived product feature
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

from tests.proxy import get_context, set_context, get_proxy_error


def test_invalid_derived_product_vendor(accelize_drm, conf_json, cred_json, async_handler,
                    log_file_factory):
    """
    Test a invalid derived product vendor returns an error
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        deriv_prod = drm_manager.get('derived_product')
        new_deriv_prod = 'a' + deriv_prod
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(derived_product=new_deriv_prod)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
        assert "Invalid derived product information: vendor mismatch" in str(excinfo.value)

    # Same test but using config file
    new_deriv_prod = 'b' + deriv_prod
    conf_json['derived_product'] = new_deriv_prod
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert "Invalid derived product information: vendor mismatch" in str(excinfo.value)


def test_invalid_derived_product_library(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test a invalid derived product library returns an error
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        deriv_prod = drm_manager.get('derived_product')
        deriv_prod_list = deriv_prod.split('/')
        deriv_prod_list[1] += 'a'
        new_deriv_prod = '/'.join(deriv_prod_list)
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(derived_product=new_deriv_prod)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
        assert "Invalid derived product information: library mismatch" in str(excinfo.value)

    # Same test but using config file
    deriv_prod_list = deriv_prod.split('/')
    deriv_prod_list[1] += 'b'
    new_deriv_prod = '/'.join(deriv_prod_list)
    conf_json['derived_product'] = new_deriv_prod
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert "Invalid derived product information: library mismatch" in str(excinfo.value)


def test_invalid_derived_product_name(accelize_drm, conf_json, cred_json,
                    async_handler):
    """
    Test a invalid derived product name returns an error
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        deriv_prod = drm_manager.get('derived_product')
        deriv_prod_list = deriv_prod.split('/')
        deriv_prod_list[2] = 'a' + deriv_prod_list[2]
        new_deriv_prod = '/'.join(deriv_prod_list)
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(derived_product=new_deriv_prod)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
        assert "Invalid derived product information: name mismatch" in str(excinfo.value)

    # Same test but using config file
    deriv_prod_list = deriv_prod.split('/')
    deriv_prod_list[2] = 'b' + deriv_prod_list[2]
    new_deriv_prod = '/'.join(deriv_prod_list)
    conf_json['derived_product'] = new_deriv_prod
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert "Invalid derived product information: name mismatch" in str(excinfo.value)


@pytest.mark.no_parallel
@pytest.mark.minimum
@pytest.mark.packages
def test_valid_derived_product(accelize_drm, conf_json, cred_json,
                    async_handler, live_server, log_file_factory, request):
    """
    Test a valid derived product behaves as expected.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        deriv_prod = drm_manager.get('derived_product')
        suffix = '_subproduct1'
        new_deriv_prod = deriv_prod + suffix
        drm_manager.set(derived_product=new_deriv_prod)
        assert drm_manager.get('derived_product') == new_deriv_prod

        # Set initial context on the live server
        context = {'product_suffix':suffix}
        set_context(context)
        assert get_context() == context
        drm_manager.activate()
        context = get_context()
        assert context['derived_product'] == new_deriv_prod
        drm_manager.deactivate()
    log_content = logfile.read()
    assert search('Loaded new derived product: %s' % new_deriv_prod, log_content, MULTILINE)
    assert get_proxy_error() is None
    async_cb.assert_NoError()

    # Same test but from config file
    suffix = '_subproduct2'
    new_deriv_prod = deriv_prod + suffix
    conf_json['derived_product'] = new_deriv_prod
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('derived_product') == new_deriv_prod

        # Set initial context on the live server
        context = {'product_suffix':suffix}
        set_context(context)
        assert get_context() == context
        drm_manager.activate()
        context = get_context()
        assert context['derived_product'] == new_deriv_prod
        drm_manager.deactivate()
    log_content = logfile.read()
    assert search('Loaded new derived product: %s' % new_deriv_prod, log_content, MULTILINE)
    assert get_proxy_error() is None
    async_cb.assert_NoError()
    logfile.remove()


def test_derived_product_during_running_session(accelize_drm, conf_json, cred_json,
                    async_handler):
    """
    Check that a derived product cannot be modified when a session is already running
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        deriv_prod = drm_manager.get('derived_product')
        new_deriv_prod = deriv_prod + '_subproduct'
        drm_manager.activate()
        # try to modify derived product
        with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
            drm_manager.set(derived_product=new_deriv_prod)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadUsage.error_code
        assert "Derived product cannot be loaded if a session is still running" in str(excinfo.value)
        drm_manager.deactivate()
        # new derived product can now be loaded
        drm_manager.set(derived_product=new_deriv_prod)
        assert drm_manager.get('derived_product') == new_deriv_prod
        async_cb.assert_Error(accelize_drm.exceptions.DRMBadUsage.error_code, 'Derived product cannot be loaded if a session is still running')
        async_cb.reset()
