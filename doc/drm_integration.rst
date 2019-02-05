Integration
============

This section explain the full integration process of the Accelize DRM.

Prerequisites
-------------

Web service access
~~~~~~~~~~~~~~~~~~

The web service access require:

* Internet connection in order to allow outbound HTTPS connection
  to `master.metering.accelize.com`_.
* Client DNS connected to internet DNS in order to be compatible with Accelize
  scalability features

.. note:: The web service access is not mandatory in the node locked mode when
          using a license already installed on the machine.

Software and hardware integration
---------------------------------

The next step is to perform DRM Hardware and software integrations.

.. toctree::
   :maxdepth: 2

   drm_hardware_integration
   drm_library_integration

The DRM licensing mode is defined during the Software integration.

Integration validation
----------------------

To check if integration is correct, follow the following steps.

Activation validation
~~~~~~~~~~~~~~~~~~~~~

Start a design in default (metered/floating) DRM mode with valid credentials
in the ``cred.json`` file:

* The design works fine and returns the following message

.. code-block:: bash

   [INFO] Starting metering session...
   [INFO] Started new metering session with sessionId A876FD1EDE47765B and set first license with duration of 15 seconds
   [INFO] Stopping metering session...
   [INFO] Stopped metering session with sessionId A876FD1EDE47765B and uploaded last metering data

Update the ``cred.json`` file with wrong credentials and restart the design in
default DRM mode:

* The design must fail with the following error message:

.. code-block:: bash

   [INFO] Resuming metering session...
   [ERROR] WSOAuth HTTP response code : 401({"error": "invalid_client"}) [errCode=10002]
   Error activating metering session

Licensing modes validation
~~~~~~~~~~~~~~~~~~~~~~~~~~

It is possible to validate the licensing mode that will be used in the
application.

Metering
^^^^^^^^

For each protected IP, depending on the pricing defined for this IP (say, C
coins per D MB processed):

* Stimulate the design so as to process an amount of data corresponding to an
  expected amount of coins as defined by your business rule (say, 10xD MB
  corresponding to 10xC coins)

* Check the coins consumption on `Accelize account metering section`_.

Floating
^^^^^^^^

* Contact Accelize and request 2 floating licenses.

* Run 2 instances of the FPGA design in parallel to use the 2 floating licenses:

  * The 2 instances must work fine.

* Run 3 instances of the FPGA design in parallel:

  * Only 2 instances must work.

Node locked
^^^^^^^^^^^

* Contact Accelize and request 1 node locked license.

* Start the FPGA design and use the node locked license.

* Kill the application that run the DRM library:

  * The design must still run.

Troubleshooting
---------------

How to check a read of DRM registers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can do a basic check on read DRM register callbacks by reading the known
value of HDK version register of the DRM controller.

This can be done using the read/write callback functions :

* Write value 0x0 to register at offset 0x0 : this will set the register page
  number to the Page 0 of registers

* Read value from register at offset 0x68 : this correspond to the version
  register, it will contain the version of the HDK used to integrate the DRM
  controller. For version 3.2.0 of the HDK this register will contain value
  0x30200.

How to check the correct license duration on Metering mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To be sure that the license duration is in line with the frequency applied in
your design:

* Launch your FPGA application (Using ``DrmManager.activate`` from the Accelize
  DRM library).

* Just after, disconnect the network. By consequence, the next license will not
  be provided to the Hardware.

* Check that the FPGA application is locked after 2x the license duration (at
  the session begin, 2 licenses are applied, so the first license duration is
  2x the license duration)

* If the duration is not correct, please check the frequency applied to the DRM
  Controller clock (DRM_aCLK)

If the duration is still not correct, you can contact Accelize
support: :doc:`contacts`

.. _Accelize account metering section: https://drmportal.accelize.com/user/metering
.. _master.metering.accelize.com: https://master.metering.accelize.com