Accelize DRM Service
====================

The advantage of the systemd approach is that:

* it runs in background, keeping the FPGA licensed at all time and independently from your application.
* it removes the ``activate`` call delay penalty on the first call by doing it at OS startup.

The Accelize DRM systemd service is an implementation of a long-running application using the
DRM library API. So if your environment is not supported yet you can easily implemented it by
your own following the instructions in :doc:`drm_library_as_api`.

.. image:: _static/Accelize_DRM_Technology_service.png
   :target: _static/Accelize_DRM_Technology_service.png
   :alt: DRM implementation as a service

Availability
------------

This service currently supports the following FPGA environments:

* AWS F1 instances.

If your configuration is not supported, contact `Accelize support <mailto:support@accelize.com>`_
for mode information.

This is an open-source project that accepts external contributions. So feel free to implement
your own configuration and if you're happy to share it with others please submit it to us.

Service configuration
---------------------

The DRM service requires ``conf.json`` and ``cred.json`` files used to configure
the DRM behavior and authenticate the user. See :doc:`drm_configuration`
for more information about configuration files.

The service also require the FPGA slot where activate license and the driver
used.

Default configuration
~~~~~~~~~~~~~~~~~~~~~

The default service configuration is the following:

* Only FPGA slot `0` is licensed.
* The configuration file is waited in ``/etc/accelize_drm/conf.json``.
* The credentials file in is waited in ``~/.accelize_drm/cred.json``
* The FPGA driver is AWS F1 instance types (`aws_f1`)

.. note:: If the service is running system-wide, it will search for
          the credential file in the home directory of the ``root`` user.

Configuration customization
~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is possible to customize the configuration by extending the service unit
file to pass following environment variables to it:

* `ACCELIZE_DRM_CRED_<Slot_Number>`: Path to `cred.json` file to use for
  the FPGA slot corresponding to `Slot_Number`.
* `ACCELIZE_DRM_CONF_<Slot_Number>`: Path to `conf.json` file to use for
  the FPGA slot corresponding to `Slot_Number`.
* `ACCELIZE_DRM_DRIVER_<Slot_Number>`: FPGA driver name to use for
  the FPGA slot corresponding to `Slot_Number`.
  Possible values: `aws_f1` for AWS F1 instances.
* `ACCELIZE_DRM_IMAGE_<Slot_Number>`: FPGA image to program in the FPGA slot
  corresponding to `Slot_Number`. If not specified, nothing is programmed.
  Possible Values: AGFI or AFI image ID for AWS F1 instances.
* `ACCELIZE_DRM_DISABLED_<Slot_Number>`: If specified, do not license the
  specified FPGA slot. This can be used to use the Service to only program the
  FPGA in the case the licensing is managed with the DRM library directly in the
  application.
* `ACCELIZE_DRM_LOG_VERBOSITY`: Log verbosity level of the DRM service log files
  in `/var/log/accelize_drm`. A rotating log file is created in this directory
  for each FPGA slot. The default level is 2 (Info). These detailed logs files
  are mainly intended to be used by Accelize support team. Use `journalctl` to
  access running service information.

A slot will be licensed if at least one of corresponding environment variable
is set.

If an environment variable is not set for a slot, the default value is used.

To do so, create a file
`/etc/systemd/system/accelize_drm.service.d/my_application.conf` and complete it
with your configuration:

.. code-block:: ini

    [Service]

    # FPGA Slot 0 configuration.
    # This slot has all variables explicitly specified
    Environment=ACCELIZE_DRM_DRIVER_0=aws_f1
    Environment=ACCELIZE_DRM_CRED_0=/root/.my_application/cred.json
    Environment=ACCELIZE_DRM_CONF_0=/etc/my_application/conf.json

    # FPGA Slot 1 configuration.
    # This slot will use default values for cred.json and conf.json paths
    Environment=ACCELIZE_DRM_DRIVER_1=aws_f1

    # ...

    # FPGA Slot 7 configuration
    # The configuration can be specified for any required slot
    Environment=ACCELIZE_DRM_DRIVER_7=aws_f1
    Environment=ACCELIZE_DRM_CRED_7=/root/.my_application/cred.json
    Environment=ACCELIZE_DRM_CONF_7=/etc/my_application/conf.json

Service usage
-------------

To start the ``systemctl`` service:

.. code-block:: bash

    # Start the service
    sudo systemctl start accelize_drm

    # Make the service automatically start on boot
    sudo systemctl enable accelize_drm
