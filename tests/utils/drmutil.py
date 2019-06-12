#! /usr/bin/env python3
# coding=utf-8
"""
Python Accelize DRM library utility. Intended to debug and testing.

Usage on AWS F1: sudo LD_LIBRARY_PATH=/path/to/drmlib/so ./drmutil.py
If make install has been done: sudo ./drmutil.py
"""

import sys, os

DIR_PATH = os.path.dirname(os.path.realpath(__file__))

# Get DRM library
sys.path.insert(0, os.path.join(DIR_PATH,os.pardir,'build/python3_bdist'))
import accelize_drm

# Get Driver
from accelize_drm.fpga_drivers import get_driver
driver_class = get_driver('aws_f1')
driver = driver_class()


if __name__ == '__main__':

    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--cred', action='store', type=str, default='cred.json', help='Specify credentials file path')
    parser.add_argument('--conf', action='store', type=str, default='conf.json', help='Specify configuraiton file path')
    args = parser.parse_args()

    # Instantiate
    err = 1
    drm_manager = None
    try:
        drm_manager = accelize_drm.DrmManager(
           args.conf, args.cred,
           driver.read_register_callback,
           driver.write_register_callback)

        val = 987654321
        drm_manager.set(custom_field=val)
        print("Wrote custom field: ", val)

        val_back = drm_manager.get('custom_field')
        print("Read back custom field: ", val_back)

        if val_back == val:
            err = 0

    except accelize_drm.exceptions.DRMException as e:
        print("Caught exception: ", str(e))

    finally:
        if err:
            print("\n*** Test failed ***\n")
        else:
            print("\n*** Test passed ***\n")
