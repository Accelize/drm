# Accelize DRM library

The Accelize DMR allow to protect FPGA applications.

The Accelize DRM library operate the DRM from the software side of the
application.

This library is responsible for activating the DRM, communicating with the
Accelize web service and managing metering sessions.

This library provide C/C++/Python API.

For more information see [Documentation](@PROJECT_DOCUMENTATION_URL@).

## Accelize DRM Service

The Python Accelize DRM package also provides a systemd service to handle the
Accelize DRM as background task. For more information, see the documentation.

## Python FPGA Drivers for Accelize DRM Python Library

This package provides examples of Python binding of FPGA drivers to use with the
Accelize DRM Python Library.

### Example of use

This example show how to use FPGA driver from this library with DrmManager.

```python
# import the library
from accelize_drm import DrmManager
from accelize_drm.fpga_drivers import get_driver

# Get the driver class by name
driver_class = get_driver('aws_f1')

# Instantiate the driver and eventually configure the FPGA with a bitstream
driver = driver_class(
    fpga_image='agfi-03be02f29cd7e466e',
    )

# Instantiate Accelize DrmManager it using the FPGA driver
drm_manager = DrmManager(
   conf_file_path='./conf.json', cred_file_path='./cred.json',

   # Read/write register callbacks from driver
   read_register=driver.read_register_callback,
   write_register=driver.write_register_callback
   )
```
