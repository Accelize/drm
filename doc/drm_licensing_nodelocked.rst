Node-locked Licenses Generation
===============================

Node-locked licenses can be generated using two different processes depending on
the target system configuration.

Online Provisioning
-------------------

The preferred way.

The online provisioning applies to every system being able to connect at least
once to the Accelize web service.

The procedure is fairly simple:

* Add credential file.
* Add the Node-locked parameters to the configuration file.
* Run your application

Refer to :doc:`drm_configuration` for information about credential and
configuration files.

At first execution, your application will connect the the Accelize web service
and download the node-locked license file in the folder configured in the
configuration file.

On every following execution, the application will load the license file from
the folder configured in the configuration file.

Offline Provisioning
--------------------

The offline provisioning only applies to completely offline systems.

The procedure is the following:

* Add the Node-locked parameters to the configuration file
  (refer to :doc:`drm_configuration`).
* Remove `url` from `licensing` section of the configuration file.
* Run your application once
* The execution will fail by lack of license file but will generate a license
  request file in the `license_dir` folder configured in the configuration
  file.
* Send the license request file to Accelize support (refer to :doc:`contacts`).
* The Accelize support will generate a license file for you.
* Copy the license file in the folder configured in the configuration file.
* Run your application.

.. note:: In a future version of the Accelize Portal, you will be able to
          generate the license file yourself by uploading the license request
          file from a dedicated menu.
