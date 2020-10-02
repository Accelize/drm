# -*- coding: utf-8 -*-
"""
Test DRM Library with bad arguments. Make sure errors are detected and reported as expected.
"""
import pytest
from re import search
from flask import request as _request
from json import dumps


@pytest.mark.minimum
def test_normal_usage(accelize_drm, request, exec_func, live_server, tmpdir):
    """Check memory leak with valgrind"""
    if 'aws' not in accelize_drm.pytest_fpga_driver_name:
        pytest.skip("C unit-tests are only supported with AWS driver.")

    # Set initial context on the live server
    nb_running = 2
    healthPeriod = 2

    context = {'healthPeriod':healthPeriod}
    set_context(context)
    assert get_context() == context

    # Create C/C++ executable
    exec_func._conf_path['licensing']['url'] = _request.url + request.function.__name__
    exec_func._conf_path.save()
    driver = accelize_drm.pytest_fpga_driver[0]
    exec_lib = exec_func.load('unittests', driver._fpga_slot_id, valgrind_log_file)

    # Run executable
    p = tmpdir.join('params.json')
    p.write("{'nb_running':%d}" % nb_running)     # Save exec parameters to file
    exec_lib.run(request.function.__name__, p)
    print('exec_lib=', str(exec_lib))
    assert exec_lib.returncode == 0
    assert 'Read register callback function must not be NULL' in exec_lib.stdout
    assert exec_lib.asyncmsg is None
