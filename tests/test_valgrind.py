# -*- coding: utf-8 -*-
"""
Test DRM Library with bad arguments. Make sure errors are detected and reported as expected.
"""
import pytest
from re import search, IGNORECASE, MULTILINE
from flask import request as _request
from json import dumps
from os.path import join, isfile

from tests.proxy import get_context, set_context


@pytest.mark.minimum
def test_normal_usage(accelize_drm, request, exec_func, live_server, tmpdir,
                      basic_log_file):
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
    exec_func._conf_json['licensing']['url'] = _request.url + request.function.__name__
    exec_func._conf_json['settings'].update(basic_log_file.create(0))
    exec_func._conf_json.save()
    driver = accelize_drm.pytest_fpga_driver[0]
    valgrind_log_file = join(accelize_drm.pytest_artifacts_dir, 'valgrind.log')
    exec_lib = exec_func.load('unittests', driver._fpga_slot_id, valgrind_log_file)

    # Run executable
    param_file = tmpdir.join('params.json')
    param_file.write('{"nb_running":%d}' % nb_running)     # Save exec parameters to file
    exec_lib.run(request.function.__name__, param_file)
    assert exec_lib.returncode == 0
    content = basic_log_file.read()
    assert search(r'DRM session \S{16} started', content, IGNORECASE)
    assert search(r'DRM session \S{16} stopped', content, IGNORECASE)
    assert search(r'\[\s*(error|critical)\s*\]', content, IGNORECASE) is None
    assert search(r'\[\s*trace\s*\]', content, IGNORECASE)
    # Analyze valgrind output file
    assert isfile(valgrind_log_file)
    with open(valgrind_log_file, 'rt') as f:
        content = f.read()
    assert search(r'definitely lost: 0 bytes in 0 blocks', content, IGNORECASE)
    assert search(r'ERROR SUMMARY: 0 errors from 0 contexts', content, IGNORECASE)


    assert search(r'^==\d+==\s+by .*drm.*', content, IGNORECASE | MULTILINE) is None
    assert search(r'^==\d+==\s+by .*accelize.*', content, IGNORECASE | MULTILINE) is None
    basic_log_file.remove()

