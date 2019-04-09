Hardware integration
====================

This section gives the main steps to generate a bitstream including the DRM
controller and Protected IPs (including the DRM Activators). The resulting
bitstream doesn't work standalone: the accelerator is locked, and a
Accelize DRM library connection is required to provide the design with a valid
license to unlock it.

For more information about DRM controller and activator:

 * :doc:`drm_hardware_ip_activator`.
 * :doc:`drm_hardware_ip_controller`.

Supported hardware
------------------

The following table lists the FPGA vendor, FPGA families and FPGA programming
tools supported by the DRM HDK.

Supported FPGA:

* Xilinx: Ultrascale+, Ultrascale, Virtex 7, Virtex 6, Spartan 6,
  Spartan 3a DSP, Spartan 3a, Kintex 7, Artix 7
* Intel: Cyclone V, Arria 10 [#f1]_, Arria V GZ, Arria V, Stratix V

.. [#f1] Node locked licensing mode not supported on
   `Intel PAC <https://www.intel.com/content/www/us/en/programmable/products/boards_and_kits/dev-kits/altera/acceleration-card-arria-10-gx.html>`_
   context, because Chip ID primitive is not reachable.

Identify how many IP cores must be protected 
--------------------------------------------

Identify how many IP cores must be protected (including IP cores that are
already protected by a DRM activator) in your FPGA design. For example,
In the following figure, 7 IP cores must be protected (3 instances of IP core A,
1 instance of IP core B, 2 instances of IP core D, 1 instance of IP core E):

.. image:: _static/Bus-architecture.png
   :target: _static/Bus-architecture.png
   :alt: alt_text

Send request to Accelize 
------------------------

Send a request for a DRM controller and DRM activators to Accelize, with the
following information:

* How many IP cores (including multiple instances of an IP core) must be
  protected in total
* How many already protected IP cores (and what IP cores) you must reuse
* For each IP core that must be protected and that is not already protected,
  provide a VLNV (Vendor name, Library name, design Name, Version number).
  For multiple instance IP cores, only one VLNV is required.

Accelize will provide you with: 

* a DRM controller with as many ports as there are protected IP instances in
  your design (including already protected IPs)
* as many DRM activators as there are new IPs to be protected in you design
  (a single DRM Activator will be delivered for multiple instances of an IP
  core)

For example, if you consider the previous figure, you will receive a DRM
Controller IP with 7 ports, and 2 DRM Activators (1 for IP core A, 1 for IP
core B).

Prepare IP cores that must be protected with an activation bus
--------------------------------------------------------------

There are 2 alternatives to protect an IP core with a DRM activator. In this
document we focus on the second option.

   #. you integrate the DRM activator in the IP core, therefore exposing only
      an interface (DRM bus interface) to interact with the DRM controller
   #. you create a wrapper, into which you instantiate a DRM activator, which
      you bind to the IP core that must be prepared to expose the following
      specific interface (activation bus) to interact with the DRM activator

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
     - IP Core control to increment the Metering counter
   * - ACTIVATION_CODE_READY
     - in
     - 1
     - the Activation Code is ready (synchronous to IP_CORE_aCLK)
   * - ACTIVATION_CODE
     - in
     - 128
     - the Activation Code (synchronous to IP_CORE_aCLK)
   * - DEMO_MODE
     - in
     - 1
     - must be set to '0'
 
Instantiate IP core and DRM Activator in a wrapper 
--------------------------------------------------

For each IP core that must be protected:

* Create a wrapper
* Instantiate the IP core (prepared with an activation bus) in the wrapper
* Expose the original IP core I/O I/F as I/O I/F of the wrapper
* Bind the original IP core I/O I/F with the corresponding I/O I/F of the
  wrapper
* Instantiate the DRM activator in the wrapper
* Bind the activation bus I/F of the DRM activator with the activation bus
  I/F of the IP core
* Expose the remaining DRM bus I/F of the DRM activator as I/O I/F of the
  wrapper
* Bind the DRM bus I/F of the DRM activator with the corresponding I/F of
  the wrapper

.. image:: _static/Protected-IP.png
   :target: _static/Protected-IP.png
   :alt: alt_text

Creating a wrapper is key since the wrapper will be encrypted so as to hide the
activation code. In the remainder of this document, a wrapper is called
"Protected IP".

Using your protected IP activation code
---------------------------------------

For each IP core that must be protected, please use the activation code provided
to you by Accelize.

The IP Core samples the 128 bits ACTIVATION_CODE when ACTIVATION_CODE_READY is
true.

The 128 bits are used to create conditions for IP features
activation/deactivation. Individual bit, groups of bits, range of bits can be
used in the IP Core code to gate signals, to switch FSM states, to select
functional parts.

Example using the 20 LSBs of the ACTIVATION_CODE signal:

* 8 bits of the Activation Code are used to unlock FSMs transitions
* 4 bits are used to control a Data Path
* 8 bits are used to enable the Feature 1 and Feature 2

.. image:: _static/Activation-code.png
   :target: _static/Activation-code.png
   :alt: alt_text

Metering Control
~~~~~~~~~~~~~~~~

The IP Core drives the DRM_EVENT signal (synchronous to IP_CORE_aCLK) to
increment the Metering counter (64 bits length).

.. warning:: Please pay particular attention to the way the IP core drives the
             DRM_EVENT signal as it is directly related to the business model
             for this IP core: 1 coin corresponds to 1 DRM event.

An IP Core reset (IP_CORE_aRSTn) resets the Metering Counter.

.. warning:: The IP core reset SHALL NOT be connected to a user-controllable
             reset as it will give the user a way to reset usage information
             before this information is sent to the DRM web service (and thus
             before invoicing the user).

Demo mode Control
~~~~~~~~~~~~~~~~~

The DEMO_MODE signal indicates that the loaded license is credit based: an
Activation timer in the IP Activator is initialized a first time, with the
value conveyed by the License, after the DRM Bus reset.

The IP Core drives the DRM_EVENT signal (synchronous to IP_CORE_aCLK) to
decrement the Activation timer (64 bits length) until exhaustion. When timeout
is reached, the Activation Code is all 0's and the signal ACTIVATION_CODE_READY
is '0'.

An IP Core reset (IP_CORE_aRSTn) is needed to enable a new initialization of
the Activation timer.

.. warning:: The IP core reset SHALL NOT be connected to a user-controllable
             reset as it will give the user a way to reset usage information
             before this information is sent to the DRM web service (and thus
             before invoicing the user).

Encrypt the wrapper 
-------------------

Encrypt each protected IP in IEEE 1735. Once encrypted, the activation bus that
is internal to the wrapper is not visible anymore.

Instantiate the DRM Controller IP 
---------------------------------

A single DRM Controller must be instantiated in FPGA to interact with multiple
protected IP cores.

* Instantiate the DRM controller in the top level design
* connect the DRM controller AXI4 lite I/F with the AXI4 lite interface of the
  top level design
* **make sure you use a correct offset address to access the DRM controller**
* connect each DRM bus I/F of the DRM controller with a DRM bus I/F of a
  protected IP core.

.. image:: _static/AXI4-bus.png
   :target: _static/AXI4-bus.png
   :alt: alt_text

Implementation
--------------

Xilinx Vivado
~~~~~~~~~~~~~

**Supported versions:** 2018.2, 2017.4, 2017.2, 2016.4, 2016.2.

For Vivado, GUI or TCL script can be used to synthesize the DRM controller and
the DRM Activator. VHDL or Verilog format can be used to be integrated.
The Verilog is a wrapper of the VHDL design.

DRM controller and DRM activators are presented independently but they can be
synthesized in the same design.

VHDL
^^^^

DRM Contoller
`````````````

GUI can be used as it in Vivado (2017.4 version) during project wizard creation:

.. image:: _static/VHDL-ctrl-vivado.png
   :target: _static/VHDL-ctrl-vivado.png
   :alt: alt_text

Or via TCL script in Vivado:

.. code-block:: tcl

   read_vhdl -library drm_library {
      drm_controller_with_dna_inst.vhdl
      xilinx/drm_all_components.vhdl
   }

The VHDL files must de compiled in "drm_library" library and the Top Level
module is: "DRM_CONTROLLER_WITH_DNA_inst"

DRM Activator
`````````````

GUI can be used as it in Vivado (2017.4 version) during project wizard creation:

.. image:: _static/VHDL-Activator-vivado.png
   :target: _static/VHDL-Activator-vivado.png
   :alt: alt_text

Or via TCL script in Vivado:

.. code-block:: tcl

   read_vhdl -library drm_library {
     drm_ip_activator_0x1000000200150001.vhdl
      ../DRM_controller/xilinx/drm_all_components.vhdl
   }

The VHDL files must de compiled in "drm_library" library and set top module is
"DRM_IP_ACTIVATOR_0x1000000200150001". 0x1000000200150001 is corresponding to
the hexadecimal value corresponding to the VLVN string. So it can differ
according the VLVN provided to Accelize to generate a DRM activator
corresponding to the desired VLVN.

Verilog
^^^^^^^

DRM Contoller
`````````````

GUI can be used as it in Vivado (2017.4 version) during project wizard creation:

.. image:: _static/Verilog-ctrl-vivado.png
   :target: _static/Verilog-ctrl-vivado.png
   :alt: alt_text

Or via TCL script:

.. code-block:: tcl

   read_verilog -library drm_library {
      drm_controller_with_dna_inst.v
   }
   read_verilog -library drm_library {
      drm_controller_with_dna_inst.vhdl
      xilinx/drm_all_components.vhdl
   }

The VHDL and Verilog files must be compiled in "drm_library" library and
the Top Level module is: "DRM_CONTROLLER_WITH_DNA_inst_wrapper".

DRM Activator
`````````````

GUI can be used as it in Vivado (2017.4 version) during project wizard creation:


.. image:: _static/Verilog-activator-vivado.png
   :target: _static/Verilog-activator-vivado.png
   :alt: alt_text

Or via TCL script:

.. code-block:: tcl

   read_verilog -library drm_library {
      drm_ip_activator_0x1000000200150001.v
   }
   read_vhdl -library drm_library {
      xilinx/drm_all_components.vhdl
      ddrm_ip_activator_0x1000000200150001.vhdl
   }

The VHDL and verilog files must be compiled in "drm_library" library and set
top module is "DRM_IP_ACTIVATOR_0x1000000200150001_wrapper". 0x1000000200150001
is corresponding to the hexadecimal value corresponding to the VLVN string.
So it can differ according the VLVN provided to Accelize to generate a DRM
activator corresponding to the desired VLVN.

Intel Quartus Prime
~~~~~~~~~~~~~~~~~~~

**Supported versions:** v18.1, v18.0, v17.1, v17.0, v16.1, v16.0

VHDL
^^^^

DRM Contoller
`````````````

GUI can be used as it in Quartus (16.1 version) during project wizard creation:


.. image:: _static/VHDL-ctrl-quartus.png
   :target: _static/VHDL-ctrl-quartus.png
   :alt: alt_text

Or via TCL script:

.. code-block:: tcl

   set_global_assignement -name TOP_LEVEL_ENTITY DRM_CONTROLLER_WITH_DNA_inst

   set_global_assignement -name VHDL_FILE alteraProprietary/drm_all_components.vhdl -library drm_library

   set_global_assignement -name VHDL_FILE src/drm_controller_with_dna_inst.vhdl -library drm_library

The VHDL files must de compiled in "drm_library" library and the Top Level
module is: "DRM_CONTROLLER_WITH_DNA_inst"

DRM Activator
`````````````

GUI can be used as it in Quartus (16.1 version) during project wizard creation:

.. image:: _static/VHDL-activator-quartus.png
   :target: _static/VHDL-activator-quartus.png
   :alt: alt_text

Or via TCL script:

.. code-block:: tcl

   set_global_assignment -name TOP_LEVEL_ENTITY DRM_IP_ACTIVATOR_0x1000000200150001

   set_global_assignment -name VHDL_FILE ../drm_controller_with_dna_inst/src/alteraProprietary/drm_all_components.vhdl -library drm_library

   set_global_assignment -name VHDL_FILE drm_ip_activator_0x1000000200150001.vhdl -library drm_library

The VHDL and Verilog files must be compiled in "drm_library" library and set
top module is "DRM_IP_ACTIVATOR_0x1000000200150001". 0x1000000200150001 is
corresponding to the hexadecimal value corresponding to the VLVN string.
So it can differ according the VLVN provided to Accelize to generate a DRM
activator corresponding to the desired VLVN.

Verilog
^^^^^^^

DRM Contoller
`````````````

GUI can be used as it in Quartus (16.1 version) during project wizard creation:

.. image:: _static/Verilog-ctrl-quartus.png
   :target: _static/Verilog-ctrl-quartus.png
   :alt: alt_text

Or via TCL script:

.. code-block:: tcl

   set_global_assignment -name TOP_LEVEL_ENTITY drm_controller_with_dna_inst_wrapper

   set_global_assignment -name VHDL_FILE ../drm_controller_with_dna_inst/src/alteraProprietary/drm_all_components.vhdl -library drm_library

   set_global_assignment -name VHDL_FILE ../drm_controller_with_dna_inst/src/drm_controller_with_dna_inst.vhdl -library drm_library

   set_global_assignment -name VERILOG_FILE ../drm_controller_with_dna_inst/src/drm_controller_with_dna_inst.v -library drm_library

The VHDL and Verilog files must be compiled in "drm_library" library and the
Top Level module is: "DRM_CONTROLLER_WITH_DNA_inst_wrapper"

DRM Activator
`````````````

GUI can be used as it in Quartus (16.1 version) during project wizard creation:

.. image:: _static/Verilog-activator-quartus.png
   :target: _static/Verilog-activator-quartus.png
   :alt: alt_text

Or via TCL script:

.. code-block:: tcl

   set_global_assignment -name TOP_LEVEL_ENTITY drm_ip_activator_0x1000000200150001_wrapper

   set_global_assignment -name VHDL_FILE ../drm_controller_with_dna_inst/src/alteraProprietary/drm_all_components.vhdl -library drm_library

   set_global_assignment -name VHDL_FILE drm_ip_activator_0x1000000200150001.vhdl -library drm_library

   set_global_assignment -name VERILOG_FILE drm_ip_activator_0x1000000200150001.v -library drm_library

   set_global_assignment -name VERILOG_FILE drm_activation_code_package_0x1000000200150001.v -library drm_library

The VHDL and Verilog files must be compiled in "drm_library" library and set
top module is "DRM_IP_ACTIVATOR_0x1000000200150001_wrapper". 0x1000000200150001
is corresponding to the hexadecimal value corresponding to the VLVN string.
So it can differ according the VLVN provided to Accelize to generate a DRM
activator corresponding to the desired VLVN.

RTL Simulation
--------------

A DRM Controller bus functional model (BFM) is provided ; it instantiates the
RTL model of the DRM Controller and implements mechanisms to load a license
file and generate signals and messages for debug

Usage
~~~~~

* Connect the DRM Bus Port of the protected IP with the DRM Bus Port of the
  DRM Controller BFM
* A default Simulation License file is embedded in the DRM Controller BFM.
  It is automatically generated and delivered in the HDK, based on the IP
  registration data (first Activation Code). If a different one is needed,
  for features level simulation for example, a new License File shall be
  explicitly requested to the DRM SaaS and assigned to the generic parameter
  LICENSE_FILE of the DRM Controller BFM
* Drive the DRM bus Clock and the DRM Bus Reset
* Observe the debug signals and messages
* Check for the IP Core features activation

.. image:: _static/RTL-simu.png
   :target: _static/RTL-simu.png
   :alt: alt_text

Expected Behavior
~~~~~~~~~~~~~~~~~

During DRM Bus reset the LICENSE_FILE_LOADED is set to '0', the
ACTIVATION_CYCLE_DONE is set to '0' and the ERROR_CODE is set to x"FF".

After DRM Bus reset, the DRM Controller BFM reads the License File and stores
it in the DRM Controller memory. When done the signal LICENSE_FILE_LOADED is set
to '1'.

In parallel, the DRM Controller runs the Activation cycle heartbeat. At the end
of the first Activation cycle, the ACTIVATION_CYCLE_DONE is set to '1' and the
ERROR_CODE is set to x"00" or x"0B" or x"0E". The value x"0B" or x"0E" means
that the License file is not yet completely written in the DRM Controller
memory, the LICENSE_FILE_LOADED being still set to '0' after the Activation
cycle start.

Ultimately, the ERROR_CODE shall be set to x"00" after a complete Activation
cycle following the LICENSE_FILE_LOADED set to '1'. If this does not happen,
the error codes can help to make decisions.

If OK, then the Protected IP shall receive its Activation Code and behave
accordingly.

.. image:: _static/behavior.png
   :target: _static/behavior.png
   :alt: alt_text

Signals for Debug (synchronous with the DRM Bus Clock)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* LICENSE_FILE_LOADED : when '1' indicates that the License file is
  loaded in the DRM Controller
* ACTIVATION_CYCLE_DONE : when '1' indicated that the DRM Controller
  has completed the first Activation cycle on the DRM Bus
* ERROR_CODE : 8 bits error code
    * x"FF" : not ready ; the DRM Controller operations are in progress
    * x"00" : no error ; the DRM Controller operations ran successfully
    * x"0B" : the License file is not conformed ; please ask for a new license
      file
    * x"0E" : the License File is corrupted ; please ask for a new license file
    * x"09", x"0F", x"10", x"11" , x"12", x"13", x"14": The DRM Controller
      cannot communicate with the IP Activator. Please check the DRM Bus
      connections, the DRM Clock generation
    * x"0A" : the DRM Controller and IP Activator versions are not compatible;
      please check that you are using the downloaded HDK without any
      modification
    * x"0C" : the DRM Controller and License File versions are not compatible ;
      please check that the right HDK version is used when asking for the
      Simulation License

Please communicate the error code to Support if assistance is needed.

ModelSim Compilation and Simulation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create libraries
^^^^^^^^^^^^^^^^

Two libraries are required : drm_library, drm_testbench_library

Library drm_library:

.. code-block:: tcl

   vlib drm_library
   vmap drm_library drm_library

Library drm_testbench_library:

.. code-block:: tcl

   vlib drm_testbench_library
   vmap drm_testbench_library drm_testbench_library 

Compile the files in the following order:
* drm_all_components.vhdl compiled in drm_library
* drm_license_package.vhdl compiled in drm_testbench_library
* drm_controller_bfm.vhdl compiled in drm_testbench_library
* drm_ip_activator_0xVVVVLLLLNNNNVVVV.vhdl compiled in any library name
 
Compile drm_all_components.vhdl:

.. code-block:: tcl

   vcom -93 -explicit -work drm_library /simu/modelsim/drm_all_components.vhdl

Compile drm_license_package.vhdl:

.. code-block:: tcl

   vcom -93 -explicit -work drm_testbench_library /simu/modelsim/drm_license_package.vhdl

Compile drm_controller_bfm.vhdl:

.. code-block:: tcl

   vcom -93 -explicit -work drm_testbench_library /simu/modelsim/drm_controller_bfm.vhdl

Compile drm_ip_activator_0xVVVVLLLLNNNNVVVV.vhdl:

.. code-block:: tcl

   vcom -93 -explicit -work  /rtl/drm_ip_activator_0xVVVVLLLLNNNNVVVV.vhdl|

Simulation
^^^^^^^^^^

Start the simulation :

.. code-block:: tcl

   vsim -L drm_library -L drm_testbench_library -L  -t 1ps *

Run the simulation:

.. code-block:: tcl

   run -all
