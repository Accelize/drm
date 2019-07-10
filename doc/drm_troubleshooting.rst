Troubleshooting
===============

This section exposes the most common questions and issues you may face and
how to come with them.
If you can't find the answer to your question please contact us: :doc:`contacts`

How to check the read/write register callbacks ?
------------------------------------------------

If you are facing an issue with the read/write register callbacks you should get
the following kind of error message:

"Failed to initialize DRM Controller"

You can perform a basic check of the read DRM register callback by reading the
DRM controller version register:

* Write value 0x0 in register at offset 0x0 : this will load the register Page 0
  of the DRM Controller containing the version register.

* Read the DRM Controller version register at offset 0x70
  For instance, the HDK version 3.2.0 is stored in the register like this: 0x30200.

How to check the correct license duration on Metering mode ?
------------------------------------------------------------

To be sure that the license duration is in line with the frequency applied in
your design:

* Launch your FPGA application (Using ``DrmManager.activate`` from the Accelize
  DRM library).

* Then, disconnect the network. The next license will not be provided to the
  Hardware.

* Check that the FPGA application is locked after 2 license durations (when the
  activate function is called 2 licenses are provisioned).

* If the duration is not correct, you should see in the log a message informing
  the detected frequency differs from value in the configuration file. You should
  also get a DRM_BadFrequency error code.

If the duration is still not correct, please contact Accelize support: :doc:`contacts`


If you get this error message: "DRM WS request failed"
------------------------------------------------------

With one of the following reasons:

- *"Unknown Product ID" <id> for N/A: Product ID <id> from license request is unknown.
  Please contact the application vendor."*
- *"Invalid Product Configuration" with {product.name} for N/A: the configuration for
  {product.name} is invalid. The collection of Activators from the license request does
  not match the expected configuration. Please contact the application vendor.*

These error messages appear when there is a discrepancy between the ID of the IPs in the
design and the IDs of the IPs in the product created on the platform.

Usually, it means that either:

- You’ve created a new product but still synthesized your design with an old DRM Controller, or
- You’ve created a new product but still synthesized your design with old IP Activators

**3 steps to identify the root cause:**

.. warning:: Please make sure to use  DRM Library version 2.2 or Higher

1. On the execution platform:

   a. Increase the logging verbosity to 1 (in the conf.json file)
   #. Run your application again, it will print some debug information in the terminal.
   #. Find the location where product info and  IP VLNV IDs are printed (see picture below)

      .. image:: _static/troubleshooting-console_vlnv_ids.png
         :target: _static/troubleshooting-console_vlnv_ids.png
         :alt: troubleshooting-console_vlnv_ids.png

   #. Note the product info and IP VLN IDs reported for later comparison (step 3)

      .. note:: Note: Index "0" always contains VLNV of the DRM Controller

#. On the vendor portal:

   a. Select “Products” section
   #. Expand the product you’re trying to use using the “+” button
   #. The IPs VLN IDs are reported between brackets in the “vln” section (see picture below)

      .. image:: _static/troubleshooting-portal_vlnv_ids.png
         :target: _static/troubleshooting-portal_vlnv_ids.png
         :alt: troubleshooting-portal_vlnv_ids.png


#. Comparison:

   a. Comparing the IDs between steps 1 & 2 should help you find the issue. If not, please send all this information to support@accelize.com for investigation.
