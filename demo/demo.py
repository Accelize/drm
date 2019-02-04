# Usage on AWS F1: sudo LD_LIBRARY_PATH=/path/to/drmlib/so python3 demo.py
# If make install has been done: sudo python3 demo.py

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
drm_manager = accelize_drm.DrmManager(
   './conf.json', './cred.json',
   driver.read_register_callback,
   driver.write_register_callback)

val = 987654321
drm_manager.set(CUSTOM_FIELD=val)
print("Wrote custom field: ", val)

val_back = drm_manager.get('CUSTOM_FIELD')
print("Read back custom field: ", val_back)

if val_back != val:
    print("Test failed")
else:
    print("Test passed")
