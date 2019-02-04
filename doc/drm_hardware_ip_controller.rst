DRM Controller IP
=================

This section has been written for design managers or FPGA designers concerning
the DRM IP Controller core.

For information about hardware integration, see :doc:`drm_hardware_integration`.

Features
--------

The DRM Controller manages the sensitive data communication between the System
Software (AXI4-Lite I/F) and the Protected IP Cores (DRM Bus I/F):

* The DRM Controller reads and interprets the encrypted License Key and
  conveys securely the Activation Codes and the Credit timers to the Protected
  IP Cores
* The DRM Controller collects the Metering Data from the Protected IP Cores and
  delivers an encrypted and authenticated Metering Data block to the System

The DRM Controller delivers to the System Software, design and chip level
information for the License Key request:

* DNA = Public Chip ID (128 bits)
* Protected IPs VLNVs (64 bits each)

Block diagram
-------------

.. image:: _static/DRM_CONTROLLER_BLOCKDIAGRAM.png
   :target: _static/DRM_CONTROLLER_BLOCKDIAGRAM.png
   :alt: DRM_CONTROLLER_BLOCKDIAGRAM.png

Interface description
---------------------

The communication on the DRM Bus uses a private protocol where the IP Activator
is a slave, the DRM Controller being the master. The DRM Bus I/F of the DRM
Controller depends on the number of Protected IPs to be connected: one common
part and one socket per Protected IP.


DRM Bus Common Part
~~~~~~~~~~~~~~~~~~~

Proprietary protocol

DRM Bus Socket per IP
~~~~~~~~~~~~~~~~~~~~~

Proprietary protocol

AXI4-Lite I/F
~~~~~~~~~~~~~~

The communication with the System is done with an AXI4-Lite slave interface
(Version: 2.0)

.. list-table::
   :header-rows: 1

   * - Name
     - Direction
     - Size
     - Description
   * - **SYS_AXI4_aCLK**
     - in
     - 1
     - AXI4 clock
   * - **SYS_AXI4_aRSTn**
     - in
     - 1
     - AXI4 asynchronous reset active low
   * - **SYS_AXI4_BUS_SLAVE_I_AW_VALID**
     - in
     - 1
     - AXI4 write address valid
   * - **SYS_AXI4_BUS_SLAVE_I_AW_ADDR**
     - in
     - 32
     - AXI4 write address value
   * - **SYS_AXI4_BUS_SLAVE_I_AW_PROT**
     - in
     - 3
     - AXI4 write address protection type
   * - **SYS_AXI4_BUS_SLAVE_O_AW_READY**
     - out
     - 1
     - AXI4 write address ready
   * - **SYS_AXI4_BUS_SLAVE_I_AR_VALID**
     - in
     - 1
     - AXI4 read address valid
   * - **SYS_AXI4_BUS_SLAVE_I_AR_ADDR**
     - in
     - 32
     - AXI4 read address value
   * - **SYS_AXI4_BUS_SLAVE_I_AR_PROT**
     - in
     - 3
     - AXI4 read address protection type
   * - **SYS_AXI4_BUS_SLAVE_O_AR_READY**
     - out
     - 1
     - AXI4 read address ready
   * - **SYS_AXI4_BUS_SLAVE_I_W_VALID**
     - in
     - 1
     - AXI4 write valid
   * - **SYS_AXI4_BUS_SLAVE_I_W_DATA**
     - in
     - 32
     - AXI4 write data value
   * - **SYS_AXI4_BUS_SLAVE_I_W_STRB**
     - in
     - 4
     - AXI4 write strobes
   * - **SYS_AXI4_BUS_SLAVE_O_W_READY**
     - out
     - 1
     - AXI4 write ready
   * - **SYS_AXI4_BUS_SLAVE_I_R_READY**
     - in
     - 1
     - AXI4 read ready
   * - **SYS_AXI4_BUS_SLAVE_O_R_VALID**
     - out
     - 1
     - AXI4 read valid
   * - **SYS_AXI4_BUS_SLAVE_O_R_DATA**
     - out
     - 32
     - AXI4 read data value
   * - **SYS_AXI4_BUS_SLAVE_O_R_RESP**
     - out
     - 2
     - AXI4 read response
   * - **SYS_AXI4_BUS_SLAVE_I_B_READY**
     - in
     - 1
     - AXI4 write response ready
   * - **SYS_AXI4_BUS_SLAVE_O_B_VALID**
     - out
     - 1
     - AXI4 write response valid
   * - **SYS_AXI4_BUS_SLAVE_O_B_RESP**
     - out
     - 2
     - AXI4 write response

DNA Register I/F
~~~~~~~~~~~~~~~~

This interface is used only if user want access to limited Chip ID FPGA
primitive. This latter is also used internally for the DRM Controller.

The DNA can be directly read with a simple register I/F synchronous with the
DRM bus clock:

.. list-table::
   :header-rows: 1

   * - Name
     - Direction
     - Size
     - Description
   * - **CHIP_DNA**
     - out
     - 128
     - DNA value
   * - **CHIP_DNA_VALID**
     - out
     - 1
     - DNA value is valid

Registers
---------

The DRM Controller Registers are written or read via the System Bus slave I/F.
These registers are accessible through DRM library. Please see
:doc:`drm_library_integration` for more information.

Implementation guidelines
-------------------------

Instantiation and connection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Only one DRM Controller can be instantiated in the Chip Design to serve multiple
Protected IPs.

* Specify the number of protected IP instances and get an appropriate
  DRM CONTROLLER; indeed, the DRM Bus I/F topology depends on the number of
  Protected IPs to be connected.
* Instantiate the DRM CONTROLLER at the top level of the design
* Assign a System Bus address to the DRM Controller with the natural generic
  parameter SYS_BUS_ADR_BEGIN
* Assign the actual size of the System Bus Address ports with the natural
  generic parameter SYS_BUS_ADR_SIZE
* Connect the DRM Controller to the System Bus
* Connect the DRM Controller and the different protected IP instances with the
  DRM Bus

Example of a DRM Environment Topology

.. image:: _static/DRM_ENVIRONMENT_TOPOLOGY.png
   :target: _static/DRM_ENVIRONMENT_TOPOLOGY.png
   :alt: DRM_ENVIRONMENT_TOPOLOGY.png

AXI4-Lite connection
^^^^^^^^^^^^^^^^^^^^

Standard Ports Connections

DRM Bus connection
^^^^^^^^^^^^^^^^^^

* Given the number of protected IP (N) connected on the DRM Bus, a DRM
  Controller component with N sockets DRM_BUS_MASTER_O_CS_n,
  DRM_BUS_MASTER_I_DAT_n, DRM_BUS_MASTER_I_ACK_n, DRM_BUS_MASTER_I_INTR_n,
  DRM_BUS_MASTER_I_STA_n, with n = 0 to N-1
* Each protected IP is connected on the DRM Bus :
    * Common connections : DRM_aCLK, DRM_aRSTn, DRM_BUS_MASTER_O_CYC,
      DRM_BUS_MASTER_O_WE, DRM_BUS_MASTER_O_ADR, DRM_BUS_MASTER_O_DAT
    * Dedicated connections to one socket : DRM_BUS_SLAVE_I_CS,
      DRM_BUS_SLAVE_O_DAT, DRM_BUS_SLAVE_O_ACK, DRM_BUS_SLAVE_O_INTR,
      DRM_BUS_SLAVE_O_STA

Implementation results
~~~~~~~~~~~~~~~~~~~~~~

Example for a DRM Controller supporting 10 IPs:

.. list-table::
   :header-rows: 1

   * - FPGA
     - LUT
     - FF
     - RAM
   * - **Kintex ultrascale+**
     - 11500
     - 6600
     - 5 of 36Kbits, 20 of 18Kbits
   * - **Kintex 7**
     - 12000
     - 6750
     - 3 of 36Kbits, 20 of 18Kbits

Timings
~~~~~~~

Below the table that list the performance of DRM Controller by Xilinx FPGA
family:

.. list-table::
   :header-rows: 1

   * - Xilinx Family
     - Frequency
     - Device documentation
   * - **ultrascale+**
     - 200MHz
     - `ds923 <https://www.xilinx.com/support/documentation/data_sheets/ds923-virtex_ultrascale-plus.pdf>`_
   * - **ultrascale**
     - 200MHz
     - `ds923 <https://www.xilinx.com/support/documentation/data_sheets/ds923-virtex_ultrascale-plus.pdf>`_
   * - **virtex 7**
     - 100MHz
     - `ds183 <https://www.xilinx.com/support/documentation/data_sheets/ds183_Virtex_7_Data_Sheet.pdf>`_
   * - **virtex 6**
     - Missing Data
     - `ds152 <https://www.xilinx.com/support/documentation/data_sheets/ds152.pdf>`_
   * - **spartan 6**
     - 2MHz
     - `ds162 <https://www.xilinx.com/support/documentation/data_sheets/ds162.pdf>`_
   * - **spartan 3a dsp**
     - 100MHz
     - `ds610 <https://www.xilinx.com/support/documentation/data_sheets/ds610.pdf>`_
   * - **spartan 3a**
     - 100MHz
     - `ds529 <https://www.xilinx.com/support/documentation/data_sheets/ds529.pdf>`_
   * - **kintex 7**
     - 100MHz
     - `ds182 <https://www.xilinx.com/support/documentation/data_sheets/ds182_Kintex_7_Data_Sheet.pdf>`_
   * - **artix 7**
     - 100MHz
     - `ds181 <https://www.xilinx.com/support/documentation/data_sheets/ds181_Artix_7_Data_Sheet.pdf>`_

Below the table that list the performance of DRM Controller by Intel FPGA
family:

.. list-table::
   :header-rows: 1

   * - Intel Family
     - Frequency
     - Device documentation
   * - **cyclone v**
     - 100MHz
     - `altchipid <https://www.intel.com/content/dam/altera-www/global/en_US/pdfs/literature/ug/altchipid.pdf>`_
   * - **arria 10**
     - 30MHz
     - `altchipid <https://www.intel.com/content/dam/altera-www/global/en_US/pdfs/literature/ug/altchipid.pdf>`_
   * - **arria v gz**
     - 100MHz
     - `altchipid <https://www.intel.com/content/dam/altera-www/global/en_US/pdfs/literature/ug/altchipid.pdf>`_
   * - **arria v**
     - 100MHz
     - `altchipid <https://www.intel.com/content/dam/altera-www/global/en_US/pdfs/literature/ug/altchipid.pdf>`_
   * - **stratix v**
     - 100MHz
     - `altchipid <https://www.intel.com/content/dam/altera-www/global/en_US/pdfs/literature/ug/altchipid.pdf>`_

Not yet supported - primitive available on FPGA but no integrated in DRM
Controller :

   * **Intel**
      * cyclone 10 gx
      * stratix 10

File structure
--------------

.. code-block:: bash

   DRM_Controller/
   -- sv/
   ---- altera/
   -------- altchip_id_arria10.sv
   ---- alteraProprietary/
   -------- altchip_id_arria10.sv
   -- vhdl/
   ---- altera/
   -------- drm_all_components.vhdl
   ---- alteraProprietary/
   -------- drm_all_components.vhdl
   ---- modelsim/
   -------- drm_all_components.vhdl
   ---- xilinx/
   -------- drm_all_components.vhdl
