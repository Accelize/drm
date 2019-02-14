# -*- coding: utf-8 -*-
"""
Check testing environment.
"""
import pytest


def test_versions_matches(accelize_drm):
    """
    Checks that the C/C++ library and the Python libraries versions matches.
    """
    api_version = accelize_drm.get_api_version()
    assert api_version.major == api_version.py_major
    assert api_version.minor == api_version.py_minor
    assert api_version.revision == api_version.py_revision
    assert api_version.commit == api_version.commit


def test_python_backend_library(accelize_drm, pytestconfig):
    """
    Checks that the Python library use the good C or C++ library as backend.
    """
    # Test: command line option passed
    assert accelize_drm.pytest_backend == pytestconfig.getoption("backend")

    # Test: imported backend
    backend = accelize_drm.get_api_version().backend
    if accelize_drm.pytest_backend == 'c':
        assert backend == 'libaccelize_drmc'
    else:
        assert backend == 'libaccelize_drm'


def test_credentials(cred_json, conf_json):
    """
    Tests if credentials in "cred.json" are valid.
    """
    from http.client import HTTPSConnection
    from json import loads

    # Checks Client ID and secret ID presence
    client_id = cred_json['client_id']
    client_secret = cred_json['client_secret']
    assert client_id
    assert client_secret

    # Check OAuth credential validity
    def get_oauth_token():
        """Get OAuth token"""
        connection = HTTPSConnection(
            conf_json['licensing']['url'].split('//', 1)[1])
        connection.request(
            "POST", "/o/token/",
            f"client_id={client_id}&client_secret={client_secret}"
            f"&grant_type=client_credentials",
            {'Content-type': 'application/x-www-form-urlencoded',
             'Accept': 'application/json'})
        return connection.getresponse()

    for i in range(3):
        response = get_oauth_token()
        status = response.status
        if status == 401 or status < 300:
            break

    if not (200 <= response.status < 300):
        pytest.fail(response.read().decode())
    assert loads(response.read()).get('access_token')


def test_fpga_driver(accelize_drm, cred_json, conf_json):
    """
    Test the driver used to perform tests.
    """
    from random import randint
    driver = accelize_drm.pytest_fpga_driver

    # Test DRM manager instantiation with driver
    drm_manager = accelize_drm.DrmManager(
        conf_json.path, cred_json.path,
        driver.read_register_callback, driver.write_register_callback)

    # Tests driver callbacks by writing/reading random values in a register
    for i in range(10):
        new_value = randint(0, 2**32 - 1)
        drm_manager.set(CUSTOM_FIELD=new_value)
        assert drm_manager.get('CUSTOM_FIELD') == new_value
