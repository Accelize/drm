#! /usr/bin/env python3
# coding=utf-8
"""
Python Accelize DRM library demo

Usage on AWS F1: sudo LD_LIBRARY_PATH=/path/to/drmlib/so ./demo.py
If make install has been done: sudo ./demo.py
"""

import sys

# Get DRM library
sys.path.insert(0, '../build/python3_bdist')
import accelize_drm

# Get Driver
sys.path.insert(0, '..')
from tests.fpga_drivers import get_driver
driver_class = get_driver('aws_f1')
driver = driver_class()

# Instantiate
err = 1
try:
    drm_manager = accelize_drm.DrmManager(
       './conf.json', './cred.json',
       driver.read_register_callback,
       driver.write_register_callback)

    val = 987654321
    drm_manager.set(custom_field=val)
    print("Wrote custom field: ", val)

    val_back = drm_manager.get('custom_field')
    print("Read back custom field: ", val_back)

    if val_back == val:
        err = 0

finally:
    if err:
        print("Test failed")
    else:
        print("Test passed")
