Configuration
=============

The DRM requires the following configuration files:

* The credentials file (``cred.json``): The end user credentials from Accelize
  portal. This file is managed by the application end user.

* The DRM Configuration file ``conf.json``: This file defines some basic
  parameters used to enable the application. This file is managed by the
  application vendor.


Credentials file
----------------

The credentials file contains the Accelize web service access keys required to
authenticate the user and grant him/her access to the application through license
generation based on the user entitlements.

To create Accelize account and get credentials:

* Create an account on `Accelize portal registration`_.
* Create an access key from your `Accelize portal account`_.

Download the credential files (``cred.json``) which looks like this:

.. code-block:: json

   {
      "client_id": "Your client id from Accelize DRM portal",
      "client_secret": "Your client secret from Accelize DRM portal"
   }

.. note:: Even in node-locked licensing mode, assuming an internet connection is valid,
          the license file can be automatically requested from the License Web Service.
          All you need is a credentials file. Once the license file is saved locally, the
          credential file can be omitted.


.. _configuration-file:

Library Configuration file
--------------------------

The ``conf.json`` configuration file content depends on the licensing mode to
use. It can be modified without rebuilding the entire application. Here is the
basic ``conf.json`` file content:

.. code-block:: json
    :caption: Metering/Floating licensing case

    {
        "licensing": {
            "url": "https://master.metering.accelize.com"
        },
        "drm": {
            "frequency_mhz": 125,
            "drm_ctrl_base_addr": 0
        },
        "design": {
            "boardType": "ISV custom data"
        }
    }

* ``url``: URL to Accelize DRM Web Service.

  .. important:: For Chinese mainland, url must be set to ``https://alibaba.metering.accelize.com:4443``.

* ``frequency_mhz``: Must be set to the effective frequency in MHz of the DRM Controller IP.
* ``drm_ctrl_base_addr``: DRM controller base address. Must be set if the the
  application is intended to work with the Accelize DRM service. Default to ``0``.
* ``boardType``: Store any string information that the ISV (IP/App vendor) might want to save
  on his/her portal database.

DRM Controller IP Frequency
~~~~~~~~~~~~~~~~~~~~~~~~~~~

From the precision of the DRM Controller frequency depends the correctness of the license timer.
The DRM Controller is embedding a frequency self-check that returns en error if the seld-evaluated
frequency differ from the one provided in the configuraiton file through the ``frequency_mhz``
parameter.
You can disable this frequency self-check by creating an additional ``bypass_frequency_detection`` in
the ``drm`` section with the value ``true``.

Node-locked parameters
~~~~~~~~~~~~~~~~~~~~~~

By default, the configuration allow metering and floating licensing. To enable
node-locked licensing, the following options must be added to the ``conf.json``
configuration file:

* ``nodelocked``: If ``true`` enable the node-locked licensing.
  If ``false`` or missing, disable the node-locked licensing and use other
  licensing modes.

* ``license_dir``: Path to an existing directory where license files are stored
  locally. This path must have read access for users that will use the
  application and load an existing license, and write access if the license file is
  automatically generated.

.. code-block:: json
    :caption: Node-locked licensing case

    {
        "licensing": {
            "url": "https://master.metering.accelize.com",
            "nodelocked": true,
            "license_dir": "~/.accelize/licenses"
        }
    }

.. note:: Once a license has been generated and a license file is in the
          ``license_dir`` directory, the ``url`` field may be omitted. Note that
          even if url value is kept, no communication with the web service is
          performed if a valid license is already installed on the machine.

logging parameters
~~~~~~~~~~~~~~~~~~

For debug purpose, one can get some trace information by adding the following section to
the configuration file:

.. code-block:: json
    :caption: Logging parameters

    {
        "settings": {
            "log_verbosity": 3,
            "log_format": "*** [%H:%M:%S %z] [thread %t] %v ***"
        }
    }

* ``log_verbosity``: Set the level of verbosity: 0=quiet, 1=error (default), 2=warning,
  3= information, 4=debug.

* ``log_format``: Set the format of trace message as a string pattern: refer to the `SPDLOG
  documentation <https://github.com/gabime/spdlog/wiki/3.-Custom-formatting>`_.


Other parameters
~~~~~~~~~~~~~~~~

Various parameters are accessible from software in read and/or write mode.
To list these parameters use the following code:

.. code-block:: c++
    :caption: C++

    drm_manager.get<std::string>( Accelize::DRMParameterKey::list_all );

.. code-block:: c
    :caption: C

    if ( DrmManager_get_string( Accelize::DRMParameterKey::list_all ) )
        fprintf( stderr, drm_manager.error_message );

.. code-block:: python
    :caption: Python

    drm_manager.get('list_all')

Some of these parameters are better explained in the :doc:`drm_sw_advanced_description`.

.. warning:: Most of these parameters are critical for a proper functioning of the system.
             It is highly recommended not to modify them. Contact the support team for
             additional information.


Configuration files storage
---------------------------

The configurations files needs to be stored on the file system of the machine
running the DRM protected application.

* The ``conf.json`` should be managed by the application vendor.
  The application user only need read access to it. On Linux, it can be stored
  in something like ``/etc/accelize_drm/conf.json`` or
  ``/etc/my_application/conf.json``.
* The ``cred.json`` should be managed by the application user. The application
  user needs to have read and write access to it. On Linux, it can be stored in
  something like ``~/.accelize_drm/cred.json`` or
  ``~/.my_application/cred.json``.

.. warning:: The credential file contain sensible information and must be stored
             in a secure way. The minimum is to ensure that access to the folder
             containing the file ``cred.json`` is only allowed to appropriate
             users only.

.. _Accelize portal registration: https://portal.accelize.com/user/register
.. _Accelize portal account: https://portal.accelize.com/front/customer/apicredential
