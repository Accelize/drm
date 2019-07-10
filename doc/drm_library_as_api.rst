Accelize DRM API
================

The Accelize DRM can be integrated into an application as a library API.

.. image:: _static/Accelize_DRM_Technology_api.png
   :target: _static/Accelize_DRM_Technology_api.png
   :alt: DRM implementation as a API library

The DRM library supports the following languages:

* C : :doc:`drm_library_api_c`
* C++ : :doc:`drm_library_api_cpp`
* Python : :doc:`drm_library_api_python`
* Cython : :doc:`drm_library_api_python`, Cython headers section (``.pxd`` files)

.. note:: No examples are provided in the Cython language, see the DRM library
          Python source code for an example of Cython integration.

Install DRM API
---------------

.. toctree::
   :maxdepth: 3

   drm_library_installation


Include DRM API
---------------

.. code-block:: c
    :caption: In C

    #include "accelize/drmc.h"

.. code-block:: c++
    :caption: In C++

    #include "accelize/drm.h"
    using namespace Accelize::DRM;

.. code-block:: python
    :caption: In Python

    from accelize_drm import DrmManager


Use DRM API in your code
------------------------

Create a DrmManager object
~~~~~~~~~~~~~~~~~~~~~~~~~~

To instantiate a DRM Manager object, the software developer needs to provide some inputs.

First, the DRM manager requires the paths to the ``conf.json`` and the ``cred.json`` files.
Those files are used to configure the DRM (i.e licensing mode) and authenticate the user.
See :doc:`drm_configuration` for more information about configuration files.

The DRM API needs to access the DRM controller registers in the design both in
read and write directions. The means to access these registers depend on the design and the driver.
So it is the user's responsibility to implement two callback functions (read and write functions)
to perform those registers access operations.

.. note:: The read and write register functions will be used asynchronously from  the rest
          of the application. Adequate protections against concurrent accesses is left to attention
          of the user.

Finally, the DRM Manager needs a third callback function to manage asynchronous errors that the DRM
thread might report. The most basic function is a call that simply displays the message.

In the following examples, The read and write registers function are named
``fpga_read_register`` and ``fpga_write_register`` and are imported from a
FPGA C library driver.

.. code-block:: c
    :caption: In C

    #include <stdio.h>

    // Define functions to read and write FPGA registers to use them as
    // callbacks in DrmManager.

    int read_register( uint32_t offset, uint32_t* value, void* user_p ) {
        return fpga_read_register( drm_controller_base_addr + offset, value );
    }

    int write_register( uint32_t offset, uint32_t value, void* user_p ) {
        return fpga_write_register( drm_controller_base_addr + offset, value );
    }

    // Define asynchronous error callback
    void asynch_error( const char* err_msg, void* user_p ) {
        fprintf( stderr, "%s", err_msg );
    }

    // Instantiate DrmManager with previously defined functions and
    // configuration files

    DrmManager *drm_manager = NULL;
    int ctx = 0;

    if ( DrmManager_alloc(
            &drm_manager,

            // Configuration files paths
            "./conf.json",
            "./cred.json",

            // Read/write register functions callbacks
            read_register,
            write_register,

            // Asynchronous error callback
            asynch_error,

            // Contextual pointer that the user can use at his/her convenience
            &ctx ) ) {
        // In the C case, the last error message is stored inside the
        // "DrmManager"
        fprintf( stderr, "%s", drm_manager->error_message );
    }

.. code-block:: c++
    :caption: In C++

    #include <iostream>
    #include <string>

    // Define functions to read and write FPGA registers and use them as
    // callbacks to instantiate the DrmManager.
    // Note: This example use C++11 Lambda function.

    DrmManager drm_manager(
        // Configuration files paths
        "./conf.json",
        "./cred.json",

        // Read/write register functions callbacks
        [&](uint32_t offset, uint32_t * value) {
            return fpga_read_register( drm_controller_base_addr + offset, value );
        },

        [&](uint32_t offset, uint32_t value) {
            return fpga_write_register( drm_controller_base_addr + offset, value );
        },

        // Asynchronous error callback
        [&](const std::string &err_msg) {
            std::cerr << err_msg << std::endl;
        }
    );

.. code-block:: python
    :caption: In Python

    # The FPGA driver is most likely written in C and read and write
    # functions can be imported using the "ctypes" module like in following
    # example
    import ctypes

    libfpga = ctypes.cdll.LoadLibrary("libfpga.so")

    fpga_read_register = libfpga.fpga_read_register
    fpga_read_register.restype = ctypes.c_int  # return code
    fpga_read_register.argtypes = (
        ctypes.c_uint32,  # offset
        ctypes.POINTER(ctypes.c_uint32)  # value
    )

    fpga_write_register = libfpga.fpga_write_register
    fpga_write_register.restype = ctypes.c_int  # return code
    fpga_write_register.argtypes = (
        ctypes.c_uint32,  # offset
        ctypes.c_uint32  # value
    )

    drm_manager = DrmManager(
        # Configuration files paths
        "./conf.json" ,
        "./cred.json" ,

        # Read/write register functions callbacks
        # Python lambda function are used to simplify code
        lambda offset, value: fpga_read_register(
            drm_controller_base_addr + offset, value),
        lambda offset, value: fpga_write_register(
            drm_controller_base_addr + offset, value),

        # Python API provides a default asynchronous error callback that
        # raise Python exceptions on error. It is possible to override it if
        # needed
        )

Activate the protected hardware
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once the ``DrmManager`` object allocated, you need to activate the protected IPs to use your design.
This is performed with the ``activate`` method.

When the function returns and the license is valid, the protected IPs are
guaranteed to be unlocked and usable.

In case of metering and floating licensing, this function spawns a thread
to keep the design unlocked and periodically send metering information
to the Accelize Web Service.

.. warning:: The activate method call may take few seconds.
             Especially on the first call due to internet and network delay.

.. code-block:: c++
    :caption: C++

    drm_manager.activate();

.. code-block:: c
    :caption: C

    if ( DrmManager_activate( drm_manager, false ) )
        fprintf( stderr, "%s", drm_manager->error_message );

.. code-block:: python
    :caption: Python

    drm_manager.activate()

Deactivate the protected hardware
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

At the end of the application execution, when the protected IP is no longer
required, the DRM must be deactivated.

When this function returns, the protected IPs are guaranteed to be locked
(Except with the node-locked licensing mode where the IP is kept unlocked).

.. note:: Calling this function at the end of your application allows you to reuse the
          same design in different licensing modes without the need to recompile.

.. warning:: In Floating or Metering mode, the deactivate method call may take some seconds due to
             internet or network delay.

.. code-block:: c++
    :caption: C++

    drm_manager.deactivate();

.. code-block:: c
    :caption: C

    if ( DrmManager_deactivate( drm_manager, false ) )
        fprintf( stderr, "%s", drm_manager->error_message );

    // In the C case, the "DrmManager" needs also to be freed to deallocate
    // associated resources.
    if ( DrmManager_free( &drm_manager ) )
        fprintf( stderr, "%s", drm_manager->error_message );

.. code-block:: python
    :caption: Python

    drm_manager.deactivate()




Compile your application
------------------------

Your application needs to be linked against the adequate
C or C++ DRM library and have the thread support enabled.

.. code-block:: bash
    :caption: C with GCC compiler

    gcc source.c -pthread -laccelize_drmc -o application

.. code-block:: bash
    :caption: C++ with GCC compiler

    g++ source.cpp -pthread -laccelize_drm -o application

.. warning:: The library requires JsonCpp that is included as ``json/json.h`` for
             portability.

             On some OS like Ubuntu or CentOS, JsonCpp include is in a
             ``jsoncpp`` subdirectory of the system include directory
             (resulting in header file path ``jsoncpp/json/json.h``).

             Make sure to specify the correct JsonCpp include path when compiling.

             .. code-block:: bash
                :caption: GCC: JsonCpp include with an absolute path specification

                gcc source.c -pthread -laccelize_drmc -o application -I/usr/include/jsoncpp

             .. code-block:: bash
                :caption: CMake: JsonCpp include path auto-detection

                find_package(PkgConfig REQUIRED)
                pkg_check_modules(JSONCPP jsoncpp)
                include_directories(${JSONCPP_INCLUDEDIR})


Advanced usage
--------------

By default, the ``activate`` call creates a new DRM session and the ``deactivate`` call
closes it.

When a session is open, a call to the Accelize Web Service is performed to:

* get a new license key to unlock the hardware design
* get a new session ID

When a session is close, a call to the Accelize Web Service is performed to:

* send the last metering data
* close the current session

In some cases, it might be useful to pause and resume a session instead of closing it, in particular
to keep the same session ID. It may also reduce the ``activate``/``deactivate``
call delay by skipping the license request to the Accelize Web Service if the license is
still valid.

.. warning:: Pausing a session at application level maintains the session open on Accelize Web Service.
             The session must be closed explicitly by the application before it terminates.

This strategy is usually not recommended but it can be useful in the following cases:

* As a general way, when there is no possibility to have a background service,
* When the call to the hardware function is short but frequent (less than the license
  expiration period).

Pausing/resuming a DRM session
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Following code snippets show how to implement pause/resume functionality:

.. code-block:: c++
    :caption: C++

    // Activate the DRM and resume the existing session if any
    drm_manager.activate( true );

    // [...]

    // Deactivate the DRM, but pause the session instead of closing it
    drm_manager.deactivate( true );

.. code-block:: c
    :caption: C

    // Activate the DRM and resume the existing session if any
    if ( DrmManager_activate( drm_manager, true ) )
        fprintf( stderr, "%s", drm_manager->error_message );

    // [...]

    // Deactivate the DRM, but pause the session instead of closing it
    if ( DrmManager_deactivate( drm_manager, true ) )
        fprintf( stderr, "%s", drm_manager->error_message );

.. code-block:: python
    :caption: Python

    # Activate the DRM and resume the existing session if any
    drm_manager.activate(True)

    # [...]

    # Deactivate the DRM, but pause the session instead of closing it
    drm_manager.deactivate(True)


Full API documentation
----------------------

For more information about the API in your favorite language, refer to :doc:`drm_library_api`.
