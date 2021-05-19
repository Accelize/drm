========================================================
Guidlines to package DRM IPs for Xilinx(R) IP Integrator
========================================================

Initial Config
==============

* Start Vivado
* "Create project"
  * "RTL Project", "Do not specify sources at this time"
  * Select your board
* Configuration:
  * "Project Manager" > "Settings" > "IP" > "Packager"
  * Check "Create archive of IP"

Packaging the DRM Controller
============================

* Start Vivado
* "Create project"
  * "RTL Project", "Do not specify sources at this time"
  * Select your board
* TCL Console:

  .. code-block:: tcl
     :caption: In TCL

     set path_to_hdl ./drm_gstarted/drm_hdk
     read_vhdl [ glob $path_to_hdl/common/vhdl/xilinx/*.vhdl ] -library drm_library
     read_vhdl $path_to_hdl/controller/rtl/core/drm_ip_controller.vhdl -library drm_library
     read_verilog -sv [glob $path_to_hdl/controller/rtl/core/*.sv]
     read_verilog -sv [glob $path_to_hdl/controller/rtl/syn/*.sv]
     set_property top top_drm_controller [current_fileset]
     update_compile_order -fileset sources_1

* Tools > Create and package New IP
  * Package current project
* TCL Console:

  .. code-block:: tcl
     :caption: In TCL

     ipx::associate_bus_interfaces -busif drm_to_uip0 -clock drm_aclk [ipx::current_core]
     ipx::associate_bus_interfaces -busif uip0_to_drm -clock drm_aclk [ipx::current_core]
     ipx::associate_bus_interfaces -busif s_axi -clock s_axi_aclk [ipx::current_core]
     ipx::infer_bus_interface s_axi_arstn xilinx.com:signal:reset_rtl:1.0 [ipx::current_core]
     ipx::associate_bus_interfaces -clock drm_aclk -reset drm_arstn -remove [ipx::current_core]
     ipx::associate_bus_interfaces -clock drm_aclk -reset drm_arstn [ipx::current_core]
     ipx::associate_bus_interfaces -clock s_axi_aclk -reset s_axi_arstn -remove [ipx::current_core]
     ipx::associate_bus_interfaces -clock s_axi_aclk -reset s_axi_arstn [ipx::current_core]

* In the Packager GUI
  * [Review and Package]
    * Click "Package IP"

Packaging the DRM Activator
===========================

* Start Vivado
* "Create project"
  * "RTL Project", "Do not specify sources at this time"
  * Select U200 board
* TCL Console (Note that 'VVVVLLLLNNNNVVVV' is specific to your DRM package and must be replaced by the appropriate value):

  .. code-block:: tcl
     :caption: In TCL

     set path_to_drm_hdk ./drm_gstarted/drm_hdk
     read_vhdl [ glob $path_to_drm_hdk/common/vhdl/xilinx/*.vhdl ] -library drm_library
     read_vhdl [ glob $path_to_drm_hdk/activator0/core/*.vhdl ] -library drm_library
     read_vhdl [ glob $path_to_drm_hdk/activator0/syn/*.vhdl ] -library drm_library
     read_verilog -sv [ glob $path_to_drm_hdk/activator0/syn/*.sv ]
     set_property top top_drm_activator_0xVVVVLLLLNNNNVVVV [current_fileset]

* Tools > Create and package New IP
  * Package current project
* TCL console:

  .. code-block:: tcl
     :caption: In TCL

     ipx::associate_bus_interfaces -busif drm_to_uip -clock drm_aclk [ipx::current_core]
     ipx::associate_bus_interfaces -busif uip_to_drm -clock drm_aclk [ipx::current_core]
     ipx::associate_bus_interfaces -clock drm_aclk -reset drm_arstn [ipx::current_core]
     ipx::infer_bus_interface drm_arstn xilinx.com:signal:reset_rtl:1.0 [ipx::current_core]
     ipx::infer_bus_interface metering_event xilinx.com:signal:data_rtl:1.0 [ipx::current_core]
     ipx::infer_bus_interface activation_code xilinx.com:signal:data_rtl:1.0 [ipx::current_core]
     ipx::associate_bus_interfaces -busif metering_event -clock ip_core_aclk [ipx::current_core]
     ipx::associate_bus_interfaces -busif activation_code -clock ip_core_aclk [ipx::current_core]

* In the Packager GUI:
  * [Review and Package]
    * Click "Package IP"

Block Design with DRM IPs
=========================

* Start Vivado
* "Create project"
  * "RTL Project", "Do not specify sources at this time"
  * Select U200 board
* Add IP Repositories:
  * "Project Manager" > "Settings" > "IP" > "Repository"
  * Add previously created IP repositories (1 for DRM, 1 for Activator)
* "Project Manager" > "IP INTEGRATOR" > "Create Block Design"
  * Add the DRM Controller and Activator IPs


.. _Accelize: https://www.accelize.com/contact-us
