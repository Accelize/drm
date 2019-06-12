Accelize DRM
============

Overview
--------

The Accelize DRM solution is based on a licensing mechanism where activation codes are
delivered to the FPGA in order to control the behavior of IP cores. In addition it collects
and stores metering information about IP usage from the FPGA.
The metering information is then used to invoice end users.

.. image:: _static/platform-overview.png
   :target: _static/platform-overview.png
   :alt: DRM implementation as a service

The Accelize DRM solution is built upon 3 main entities:

* The DRM HDK: a set of dedicated hardware IPs used to instrument the
  targeted HW
* The DRM Library: a C/C++/Python library to communicate between the
  targeted DRM Hardware IPs and the DRM web service
* The DRM Web service: a Web Service application with a database
  (cloud hosted or on premise) to deliver the license keys and to store
  hardware usage.

Accelize operates the licensing/metering service for your FPGA solution
deployed in your public Cloud, private Cloud, on-premise or hybrid multi-Cloud
infrastructure. This allows you to implement any business model securely.
Then you bill your customers according to their usage reported to you by
Accelize. The following table gives an overview of the business models supported
by Accelize.

Here's a short `video <https://www.youtube.com/watch?v=7cb_ksLTcRk>`_ presenting
an overview of the Accelize Platform and its integration.

Licensing Modes
---------------

Overview
~~~~~~~~

The Accelize DRM enables following licensing modes:

* Metering, Data usage-based:
    Monetize your FPGA design or IP based on the data it processes
    (# of GBytes, # of frames, # of inferences, # of API calls, ...)

* Metering, Time-based:
    Monetize your FPGA design or IP based on the operating time
    (seconds, minutes, hours, days…)

* Floating:
    Enable the use of X number of FPGA designs or IPs on a pool of FPGA boards
    in a hybrid cloud infrastructure (distributed across multiple public/private
    data centers)

* Node-locked:
    Deploy your FPGA design in the form of an *appliance* by assigning a
    license to a unique board id.

Technical Description
~~~~~~~~~~~~~~~~~~~~~

* In Metering mode:
    A session is kept active as long as:

    #. a valid License Key and a License Timer is sent to the DRM Controller in the FPGA design, and
    #. the authenticated Metering Data blocks can be collected from the DRM Controller in the FPGA.

    The License Timer initialization value determines the duration of activation period and the
    Metering Data collection frequency. When the License Timer exhausts, the DRM Controller
    deactivates the IP Cores until a new valid License is loaded in the DRM Controller.
    The DRM Web Service delivers a new valid license only if an authenticated Metering Data block
    is provided within the License request. This mechanism enforces a periodic Metering Data
    collection during the Hardware operations.

* In Floating mode:
    A stream of time-based License Keys per circuit is used to provide Activation Codes and License
    Timers to the Protected IPs. The License Key generation is based on the Public Chip ID and a random
    seed generated at runtime after reset. The License Key cannot be provisioned and can be used
    only during the current runtime, the DRM Controller keeping the random seed value until the
    next reset.

* In Node-locked mode:
    A single License Key per circuit is used to provide a unique Activation Code to the Protected IPs.
    The License Key generation is based on the Public Chip ID. The License Key can be stored locally
    and reloaded at runtime whenever needed during the circuit lifecycle. Optionally the License Key
    can contain a License Timer for each IP to time limit the usage like for demo or evaluation
    licenses.

Glossary
--------

.. list-table::
   :header-rows: 1

   * - Word
     - Description
   * - **AC**
     - Activation Code
   * - **DRM**
     - Digital Rights Management
   * - **DNA**
     - A unique chip identifier; could be deliver by a PUF
   * - **PUF**
     - Physically Unclonable Function
   * - **VLNV**
     - Vendor Library Name Version
   * - **IP Core**
     - Functional block to be protected
   * - **Protected IP**
     - IP Core + DRM instrumentation
   * - **DRM Enabled IP**
     - a Protected IP
   * - **CDC**
     - Clock Domain Crossing
   * - **Credit Timer**
     - For Activation Duration controlled by the IP Activator, one per Protected IP


.. toctree::
   :maxdepth: 0
   :caption: Overview

   index

.. toctree::
   :maxdepth: 3
   :caption: Getting Started

   drm_getting_started


.. toctree::
   :maxdepth: 3
   :caption: DRM-HDK Reference Manual

   drm_hardware_ip_activator
   drm_hardware_ip_controller
   drm_hardware_integration


.. toctree::
   :maxdepth: 3
   :caption: DRM-Library Reference Manual

   drm_library_installation
   drm_library_integration
   drm_library_api
   drm_configuration


.. toctree::
   :maxdepth: 3
   :caption: DRM Advanced Description

   drm_sw_advanced_description


.. toctree::
   :maxdepth: 3
   :caption: Links

   contacts
   Accelize Website <https://www.accelize.com>
   DRM Library on Github <https://github.com/Accelize/drmlib>

Indexes and tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
