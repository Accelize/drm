DRM Activator IP
================

This section has been written for design managers or FPGA designers concerning
the DRM IP Activator core.

For information about hardware integration, see :doc:`drm_hardware_integration`.

Features
--------

The DRM IP Activator provides the following features:

   * delivers a 128 bits Activation Code to the IP Core for behavior control
   * maintains a credit timer for time based activation
   * stores a metering counter for activities measurement

Block diagram
-------------

.. image:: _static/IP_ACTIVATOR_BLOCKDIAGRAM.png
   :target: _static/IP_ACTIVATOR_BLOCKDIAGRAM.png
   :alt: IP_ACTIVATOR_BLOCKDIAGRAM.png

Operations
----------

All signals of the IP Core I/F are synchronized with the IP Core clock domain
(internal CDC with the DRM Clock domain in the IP Activator).

The IP Core captures the Activation Code when the Activation Code Ready output
is '1'.

The IP Core uses the 128 bits of the Activation Code output to control its
features enabling.

An internal Credit Timer is used for Demo License or Simulation License: its
initialization is done by the DRM IPs with the value provided in the license
file and its decrement is under the control of the IP Core by setting the input
DRM_EVENT to '1'. The IP Core knows that a temporary Activation Code has been
loaded in the IP Activator when the DEMO_MODE output is set to '1'. The IP Core
detects the Activation timeout when the Activation Code Ready output is '0'
and/or the Activation Code output is all 0’s.

An internal Metering counter is used to store the activity of the IP Core: it
is asynchronously reset upon the IP_CORE_aRSTn input and is synchronously reset
by the DRM Controller via the DRM Bus; its increment is under the control of
the IP Core by setting the input DRM_EVENT to '1'.

Interface description
---------------------

The communication on the DRM Bus uses a proprietary protocol where the IP
Activator is a slave, the DRM Controller being the master.

DRM Bus Socket connected to DRM controller
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Proprietary protocol

DRM Activator interface with IP to protect
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The interface with the IP Core is a simple register interface with control
signals.

.. list-table::
   :header-rows: 1

   * - Name
     - Direction
     - Size
     - Description
   * - IP_CORE_aCLK
     - in
     - 1
     - IP Core clock
   * - IP_CORE_aRSTn
     - in
     - 1
     - IP Core Asynchronous Reset (active low)
   * - DRM_EVENT
     - in
     - 1
     - Decrements the Credit timer and Increments the Metering counter when set to '1'; Synchronous with IP_CORE_aCLK
   * - ACTIVATION_CODE
     - out
     - 128
     - The Activation Code (synchronous to IP_CORE_aCLK)
   * - ACTIVATION_CODE_READY
     - out
     - 1
     - Tells that an Activation Code is available when set to '1'
   * - DEMO_MODE
     - out
     - 1
     - Tells that the Activation Code is temporary (synchronous to IP_CORE_aCLK)

Implementation guidelines
-------------------------

IP core instrumentation
~~~~~~~~~~~~~~~~~~~~~~~~

A protected IP, or DRM Enabled IP, consists of the assembly of an IP Core and
the IP Activator:

.. image:: _static/IP_CORE_INSTRUMENTATION.png
   :target: _static/IP_CORE_INSTRUMENTATION.png
   :alt: IP_CORE_INSTRUMENTATION.png

with the IP Activator to retrieve its activation code and to control the
activation duration and the metering increment.

The IP Vendor designs the IP core behavior based on the Activation Code values
(128 bits) and its validity.

Reset and Clock
^^^^^^^^^^^^^^^

The IP Core clock domain and the DRM Bus clock domain can be asynchronous,
the IP Activator component implements the CDC (Clock Domain Crossing).

The resets are active low and asynchronous with the clocks.

IP Activator Interface
^^^^^^^^^^^^^^^^^^^^^^

A specific interface shall be prepared in the IP Core to further communicate
with the IP Activator.

.. list-table::
   :header-rows: 1

   * - Name
     - Direction
     - Size
     - Description
   * - IP_CORE_aCLK
     - out
     - 1
     - IP Core clock domain
   * - IP_CORE_aRSTn
     - out
     - 1
     - IP Core Asynchronous Reset (active low)
   * - DRM_EVENT
     - out
     - 1
     - IP Core control to decrement an Activation timer and increment the Meterin counter
   * - ACTIVATION_CODE
     - in
     - 128
     - The Activation Code (synchronous to IP_CORE_aCLK)
   * - ACTIVATION_CODE_READY
     - in
     - 1
     - the Activation Code is ready (synchronous to IP_CORE_aCLK)
   * - DEMO_MODE
     - in
     - 1
     - the license is credit based i.e. temporary (synchronous to IP_CORE_aCLK)

Implementation results
^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1

   * - FPGA
     - LUT
     - FF
     - RAM
   * - **Kintex ultrascale+**
     - 2200
     - 1800
     - 2 of 36Kbits, 18 of 18Kbits
   * - **Kintex 7**
     - 2400
     - 1900
     - 18 of 36Kbits

File structure
--------------

The IP HDK contains the components and models for compilation and simulation of
the assembly IP core + IP Activator.

IP Activator HDK directories and files:

.. code-block:: bash

   drm_ip_activator_hdk_x.y.z/
   -- rtl/
   ---- altera/
         drm_all_components.vhdl
   ---- xilinx/
   -------- drm_all_components.vhdl
   -------- drm_ip_activator_0xvvvvllllnnnnvvvv.vhdl
   -------- drm_ip_activator_0xvvvvllllnnnnvvvv.vho
   -------- drm_ip_activator_0xvvvvllllnnnnvvvv.veo
   -------- drm_ip_activator_0xvvvvllllnnnnvvvv.v
   -------- drm_ip_activator_0xvvvvllllnnnnvvvv.xml
   -------- drm_activation_code_package_0xvvvvllllnnnnvvvv.vhdl
   -------- drm_activation_code_package_0xvvvvllllnnnnvvvv.v
   -- simu/
   ---- modelsim/
   -------- drm_all_components.vhdl
   -------- drm_controller_bfm.vhdl
   -------- drm_controller_bfm.v
   -------- drm_license_package.vhdl
   -- docs/

The archive file name postfix x.y.z is  the version number of the HDK.

For example "2.1.1.".
**This version number must be specified when asking for aLicense File**.

The IP Activator entity name is DRM_IP_ACTIVATOR_0xVVVVLLLLNNNNVVVV with the
postfix being a 64 bits hexadecimal encoding of the IP VLNV.

For example DRM_IP_ACTIVATOR_0x0C001020A56E0001

Release Note
------------

* 3.0.0.0:
    * Naming convention
* 2.3.2.4:
    * Timing closure enhancement to work at 200 MHz
* 2.3.2.3:
    * Refine DRM bus logic port naming conventions in IPXact abstract bus
      definition
* 2.3.2.2:
    * Usage of aes128-cbc for P1735 encrypted IPs.
* 2.3.2.1:
    * Split the DRM Bus IPXact abstract bus definition
    * Define a single common DRM Bus section to connect the DRM Controller to
      all IP Activators.
    * Define a dedicated DRM Bus section (aka. socket) to connect the DRM
      Controller to each IP Activator (one dedicated socket per IP Activator)
