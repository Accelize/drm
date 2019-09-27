DRM Library integration
========================

This section explains how to install and use the DRM library from your software.

Accelize DRM Library can be used as a software API that your application can call, or
you can install a systemd service but only for few environments.

However the systemd service cannot be used if the user application:

* Reprogram the FPGA
* Reset the FPGA.

Typically an OpenCL application is not compatible with this service.

In this situation the DRM API must be implemented in the user application.


.. toctree::
   :maxdepth: 3
   :caption: DRM-Library as an API

   drm_library_as_api

.. toctree::
   :maxdepth: 3
   :caption: DRM-Library as a systemd service

   drm_library_as_service
