Getting started
===============

This section explains how your application can be integrated into the
Accelize DRM Distribution platform.

For a quick tour about the Accelize Platform integration steps, please watch `this video
<https://www.youtube.com/watch?v=7cb_ksLTcRk>`_.

Prerequisites
-------------

- To access the Accelize Web Service you need an Internet connection which allows
  outbound HTTPS connection to Accelize server: https://master.metering.accelize.com.

  .. note::
     In node-locked mode, the web service access is not mandatory. The License Key file
     will just have to be manually copied on the machine (refer to `Node-locked`_).

  .. note::
     The DRM licensing mode is defined during the `configuration mode <drm_configuration>`_.

- Create your account from `Accelize website <https://www.accelize.com/content/request-vendor-account>`_
  web site.

.. warning::
   DRM API 1.x is compatible with the DRM HDK versions inferior or equal to 2.x.
   For DRM HDK superior to 3.x use exclusively DRM API 2.x.

Adapt your FPGA design
----------------------

This paragraph offers an overview of hardware integration steps. For the
complete documentation, refer to :doc:`drm_hardware_integration`.

Modify your FPGA design
~~~~~~~~~~~~~~~~~~~~~~~

You need to modify your FPGA design to include the DRM HDK.

1. Contact `Accelize <https://www.accelize.com/contact-us>`_ to download the DRM HDK and perform
   the basic settings for you to test your FPGA design.
#. Instantiate the DRM Controller in the design top-level.
#. Instantiate the DRM Activator in the IP/design to protect.
#. Develop the piece of logic that lock and unlock your design based on
   the activation code value.
   (Find full documentation in the section 'Protect relevant code of the IP core' of
   :doc:`drm_hardware_integration`.)
#. Develop the piece of logic that generates a pulse on the ``metering_event`` port of
   DRM Activator each time the usage unit of your choice (bytes, frame, ...) is hit.
   (Find full documentation in the section 'Add metering logic' of :doc:`drm_hardware_integration` page.)
#. Connect the DRM bus between the Controller and and the Activator.
#. Connect the other port to the rest of your design.
#. Encrypt your source file to hide the activation code.

Synthesize and implement your FPGA design
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Add the DRM HDK files to your synthesis script.

   .. warning::
      DRM Conbtroller VHDL source files must be compiled under the library ``drm_library`` and
      DRM Activator VHDL source files must be compiled under the library ``drm_0xVVVVLLLLNNNNVVVV_library``
      or the synthesis will fail.

2. To simulate your design, use the simulation source file in the HDK.
   For more details, visit the section 'RTL Simulation' of :doc:`drm_hardware_integration` page.

For more details, please refer to the section 'Synthesize and implement your design' in
:doc:`drm_hardware_integration` page.

Adapt your software application
--------------------------------

For more details, please refer to the :doc:`drm_library_integration` page.


Validate your integration
-------------------------

Check design activation
~~~~~~~~~~~~~~~~~~~~~~~

To check the integration is correct, perform the following operations:

1. Check design is activated:

   Start a design in default (metered/floating) DRM mode with valid credentials
   in the ``cred.json`` file.
   The design works fine and should return the following message:

   .. code-block:: bash

      [INFO] Starting metering session...
      [INFO] Started new metering session with sessionId A876FD1EDE47765B and set first license with duration of 15 seconds
      [INFO] Stopping metering session...
      [INFO] Stopped metering session with sessionId A876FD1EDE47765B and uploaded last metering data

2. Check design cannot be activated:

   Update the ``cred.json`` file with wrong credentials and restart the design in
   default DRM mode:
   The design should fail with the following error message:

   .. code-block:: bash

      [INFO] Resuming metering session...
      [ERROR] WSOAuth HTTP response code : 401({"error": "invalid_client"}) [errCode=10002]
      Error activating metering session

Check licensing modes
~~~~~~~~~~~~~~~~~~~~~

It is possible to validate the licensing mode that will be used in the
application.

Metering
^^^^^^^^

Assuming the pricing plan for the design is C usage units generated every D MB of data processed.

1. Stimulate the design so that a minimum of D MB of data have been processed.

2. Open the usage page on your account: https://<your-company-name>.accelize.com/front/vendorusage
   and check the number of usage units consumed have been correctly incremented.

For instance, let's assume 1 usage unit is generated every 10 MB of data processed, C=1 and D=10.
If 10xD have been processed, you should see 10 usage units on your account.

Floating
^^^^^^^^

1. Contact Accelize and request 2 floating licenses.
2. Run 2 instances of the FPGA design in parallel to consume the 2 floating licenses.
3. Run 3 instances of the FPGA design in parallel:

  * Only 2 instances must work.

Node-locked
^^^^^^^^^^^

* Contact Accelize and request 1 node-locked license.

* Start the FPGA design and use the node-locked license.

* Kill the application that run the DRM library:

  * The design must still run.


Getting Started Examples
------------------------

A list of basic example designs are available `here <https://github.com/Accelize/GettingStarted_Examples>`_.
