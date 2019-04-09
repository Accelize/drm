# Python FPGA Drivers for Accelize DRM Python Library

This package provides examples of Python binding of FPGA drivers to use with the
Accelize DRM Python Library.

## Example of use

This example show how to use FPGA driver from this library with DrmManager.

```python
# import the library
from python_fpga_drivers import get_driver

# Get the driver class by name
driver_class = get_driver('aws_f1')

# Instantiate the class and eventually configure FPGA
driver = driver_class(
    fpga_image='agfi-03be02f29cd7e466e',
    )

# Import Accelize DrmManager and instantiate it using FPGA driver
from accelize_drm import DrmManager

drm_manager = DrmManager(
   conf_file_path='./conf.json', cred_file_path='./cred.json',
   # Read/write register callbacks from driver
   read_register=driver.read_register_callback,
   write_register=driver.write_register_callback
   )
```
