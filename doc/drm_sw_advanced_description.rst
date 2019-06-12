DRM SW Advanced Description
===========================

Retry mechanism
---------------

There are 2 types of retry mechanisms:

* One is occurring on the first license request
* The other takes place on license request renewal

On the First request
~~~~~~~~~~~~~~~~~~~~

By default a retry mechanism is implemented when the ``activate`` function is called.

The retry definition has 2 parameters:

- **ws_retry_period_short**: wait period in seconds before a new attempt is performed;
                             set to 2s by default
- **ws_request_timeout**: period in seconds during which the retry takes place;
                          set to 10s by default

.. image:: _static/retry_mechanism_first_license.png
   :target: _static/retry_mechanism_first_license.png
   :alt: Retry on first license request

.. note:: These parameter values can be changed using the configuration file or the code.

On the Other requests
~~~~~~~~~~~~~~~~~~~~~

By default a retry mechanism is implemented in the background thread when a new license needs
to be renewed.

The retry definition has 2 parameters:

- **ws_retry_period_long**: large wait period in seconds before a new attempt is performed;
                            set to 60s by default
- **ws_retry_period_short**: short wait period in seconds before a new attempt is performed;
                             set to 2s by default

The retry period starts when the DRM Controller is ready to receive a new license and lasts
until the license expires:

- The retry periodicity is initially set to ws_retry_period_long until the license
  expiration time is inferior to ws_retry_period_long
- Then the retry periodicity is set to ws_retry_period_short until the license expiration
  occurred.

.. image:: _static/retry_mechanism_license_renewal.png
   :target: _static/retry_mechanism_license_renewal.png
   :alt: Retry on license request renewal

.. note:: These parameters can be changed using the configuration file or the code.
