Configuration
=============

The DRM requires the following configuration files:

* The credentials file (``cred.json``): The user Accelize credentials.
  This file should be managed by the application user.
* The DRM Configuration file ``conf.json``: This file allow to define
  the DRM configuration. This file is managed by the application vendor.

Credentials file
----------------

The credentials file contain Accelize web service access key and is used to
authenticate the user running the application and allow or not license
generation based on user entitlements.

To create Accelize account and get credentials:

* Create an account on `Accelize DRM portal registration`_.
* Create an access key from your `Accelize account`_.

Download the credential files (`cred.json`), you should obtain a file like this:

.. code-block:: json

   {
      "client_id": "Your client id from Accelize DRM portal",
      "client_secret": "Your client secret from Accelize DRM portal"
   }

.. note:: In node locked licensing mode, the credentials configuration file is
          only accessed on license provisioning, and may be omitted if a
          license file is already installed on the machine.

DRM Configuration file
----------------------

The ``conf.json`` configuration file content depends on the licensing mode to
use and can be changed without rebuilding the entire application. Here is the
basic ``conf.json`` file content:

.. code-block:: json
    :caption: Metering/Floating licensing case

    {
        "licensing": {
            "url": "https://master.metering.accelize.com",
        }
    }

Node locked mode configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By default, the configuration allow metering and floating licensing. To enable
node locked licensing, the following options must be added to the ``conf.json``
configuration file:

* ``nodelocked``: If ``true`` enable the node locked licensing.
  If ``false`` or missing, disable the node locked licensing and use other
  licensing modes.
* ``license_dir``: Path to an existing directory where licenses files are stored
  locally. This path must have read access for users that will use the
  application and load an existing license, and write access if there is need to
  generate the license file.

.. code-block:: json
    :caption: Node locked licensing case

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

Configuration files storage
---------------------------

The configurations files needs to be stored on the file system of the machine
running the DRM protected application.

* The ``conf.json`` should be managed by the application vendor.
  The application user only need read access to it. On Linux, it can be stored
  in something like ``/etc/my_application/``.
* The ``cred.json`` should be managed by the application user. The application
  user needs to have read and write access to it. On Linux, it can be stored in
  something line ``~/.my_application``.

.. _Accelize DRM portal registration: https://drmportal.accelize.com/user/register
.. _Accelize account: https://drmportal.accelize.com/user/applications
