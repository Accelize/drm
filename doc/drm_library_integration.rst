Software integration
=====================

This section explain how to integrate the DRM in the software application.

The DRM library support following languages:

* C : :doc:`drm_library_api_c`
* C++ : :doc:`drm_library_api_cpp`
* Python : :doc:`drm_library_api_python`
* Cython : :doc:`drm_library_api_python`, Cython headers section (``.pxd``
  files)

No examples are provided in the Cython language, see the DRM library
Python source code for an example of Cython integration.

Get the Accelize DRM library
----------------------------

The Accelize DRM library is required to integrate DRM.

See :doc:`drm_library_installation` for information about installation or
building of the Accelize DRM library.

* For Cor C++ integration, the development package is required.
* For Python integration, the Python package is required.
* For Cython integration, the Python package and the development package are
  required.

Include the Accelize DRM library
--------------------------------

The Accelize DRM library must first be included:

.. code-block:: c++
   :caption: C++

   include "accelize/drm.h"
   using namespace Accelize::DRM;

.. code-block:: c
   :caption: C

   include "accelize/drmc.h"

.. code-block:: python
   :caption: Python

   from accelize_drm import DrmManager

Allocate the DrmManager
-----------------------

The DRM library needs to access DRM controller registers on the design in both
read and write directions. The library users has to provide two callback
functions to perform those operations.

.. note:: The read and write registers functions will be used asynchronously to
          the rest of the application.
          Adequate protections against concurrent accesses is required.

In following examples, The read and write registers function are named
``fpga_read_register`` and ``fpga_write_register`` and are imported from a
FPGA driver C library.

The DRM library API also needs a third callback function for managing
asynchronous errors from the DRM thread.

The DRM manager also requires paths to ``conf.json`` and ``cred.json`` files
used to configure the DRM and authenticate user. See :doc:`drm_configuration`
for more information about configuration files.

.. code-block:: c++
    :caption: C++

    // Define functions to read and write FPGA registers and use them as
    // callback to instantiate the DrmManager.
    // Note: This example use C++ 11 Lambda function to simplify code.

    DrmManager drm_manager(
        // Configuration files paths
        "./conf.json" ,
        "./cred.json" ,

        // Read/write register functions callbacks
        [&](uint32_t offset, uint32_t * value) {
            return fpga_read_register(
                drm_controller_base_addr + offset, value);
        },
        [&](uint32_t offset, uint32_t value) {
            return fpga_write_register(
                drm_controller_base_addr + offset, value);
        },

        // Asynchronous error callback
        [&](const std::string &err_msg) {
            std::cerr << err_msg << std::endl;
        }
    );

.. code-block:: c
    :caption: C

    // Define functions to read and write FPGA registers to use them as
    // callbacks in DrmManager.

    int read_register(uint32_t offset, uint32_t* value, void* user_p){
        return fpga_read_register(
            drm_controller_base_addr + offset, value);
    }

    int write_register(uint32_t offset, uint32_t value, void* user_p){
        return fpga_write_register(
            drm_controller_base_addr + offset, value);
    }

    // Define asynchronous error callback
    void asynch_error(const char* err_msg){
        std::cerr << err_msg << std::endl;
    }

    // Instantiate DrmManager with previously defined functions and
    // configuration files

    DrmManager *drm_manager = NULL;
    int ctx = 0;

    DrmManager_alloc(
        &drm_manager,

        // Configuration files paths
        "./conf.json",
        "./cred.json",

        // Read/write register functions callbacks
        read_register, write_register,

        // Asynchronous error callback
        asynch_error,

        &ctx
    );

.. code-block:: python
    :caption: Python

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

Activate the DRM
----------------

Once the ``DrmManager`` is allocated, it is needed to activate the DRM and
start using the protected IPs. This is performed with the ``activate`` method
Once this function returns successfully, your IPs are unlocked and usable.

When this function returns and the license is valid, the protected IPs are
guaranteed to be unlocked.

In case of metering and floating licensing, this function spawns a thread
to keep the design unlocked and periodically send metering information
to the Accelize Web Service.

.. warning:: The activate method call may take some seconds.
             Especially on the first call due to internet and network delay.

.. code-block:: c++
    :caption: C++

    drm_manager.activate();

.. code-block:: c
    :caption: C

    if DrmManager_activate(drm_manager, false) {
        // "DrmManager_get" is used to retrieve the error message linked to
        // the return code in case of error
        char* message = NULL;
        DrmManager_get_string(drm_manager, DRM__strerror, &message);
        fprintf(stderr, error_message);
    }

.. code-block:: python
    :caption: Python

    drm_manager.activate()

Deactivate the DRM
------------------

At the end of the application execution, when the protected IP is no longer
required, the DRM must be deactivated.

When this function returns, the protected IPs are guaranteed to be locked
(Except with the node locked licensing mode where the IP is kept unlocked).

.. note:: Implementing this function at the application end allow to use the
          application with all licensing modes without the need of
          recompilation.

.. warning:: The deactivate method call may take some seconds.

.. code-block:: c++
    :caption: C++

    drm_manager.deactivate();

.. code-block:: c
    :caption: C

    if DrmManager_deactivate(drm_manager, false) {
        char* message = NULL;
        DrmManager_get_string(drm_manager, DRM__strerror, &message);
        fprintf(stderr, error_message);
    }

    // In the C case, the "DrmManager" needs also to be freed to deallocate
    // associated resources.
    if DrmManager_free(drm_manager) {
        char* message = NULL;
        DrmManager_get_string(drm_manager, DRM__strerror, &message);
        fprintf(stderr, error_message);
    }

.. code-block:: python
    :caption: Python

    drm_manager.deactivate()

Application compilation
-----------------------

At the compilation step, the application need to be linked against the adequate
C or C++ DRM library and have the thread support enabled.

.. code-block:: bash
    :caption: C with GCC compiler

    gcc source.c -pthread -laccelize_drmc -o application

.. code-block:: bash
    :caption: C++ with GCC compiler

    g++ source.cpp -pthread -laccelize_drm -o application

DRM management integration strategies
-------------------------------------

DRM manager service
~~~~~~~~~~~~~~~~~~~

This strategy consist to integrate the DRM manager in a background service.

The advantage of this strategy is to have the FPGA design always ready for
application and to avoid the first ``activate`` call delay penalty by having
it on the start of the OS.

This is the recommended strategy.

DRM manager in application
~~~~~~~~~~~~~~~~~~~~~~~~~~

This strategy consist to integrate the DRM manager directly in the application
and ``activate`` the DRM on application start and ``deactivate`` the DRM on
application exit.

To reduce the ``activate``/``deactivate`` calls delays between sparse calls of
the application, it is possible to pause and resume a DRM session. This allow
to avoid the web service call if a license is already activated in the DRM
controller.

By default, the DRM create a new DRM session on ``activate`` call and close it
on ``deactivate`` call. Closing a session means that the last metering data will
be sent to the Accelize Web Service and this will close the session ID.

In some case, it may be also usefull to pause the session instead of closing it
with ``deactivate`` and resuming the session with ``activate``.

Pausing a session means that the session can be resumed as is. The next metering
data will be accumulated to the current data. This also means that this metering
session is left opened on the Accelize Web Service (you will have to eventually
close it).

This strategy is only recommended when application is used as
sparse executable calls and there is no possibility to have a background
service to keep the DRM activated.

.. warning:: If the license in the DRM controller is expired (By example: If the
             application has not be run during a long time), ``activate`` will
             perform a web service call that may add some delay to the
             application starting delay. This can have an impact on short run
             applications.

Pausing/resuming a DRM session
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Below codes examples show how to implement pause/resume DRM session.

.. code-block:: c++
    :caption: C++

    // Activate the DRM and resume the existing session if any
    drm_manager.activate(true);

    // [...]

    // Deactivate the DRM, but pause the session instead of closing it
    drm_manager.deactivate(true);

.. code-block:: c
    :caption: C

    // Activate the DRM and resume the existing session if any
    if DrmManager_activate(drm_manager, true) {
        char* message = NULL;
        DrmManager_get_string(drm_manager, DRM__strerror, &message);
        fprintf(stderr, error_message);
    }

    // [...]

    // Deactivate the DRM, but pause the session instead of closing it
    if DrmManager_deactivate(drm_manager, true) {
        char* message = NULL;
        DrmManager_get_string(drm_manager, DRM__strerror, &message);
        fprintf(stderr, error_message);
    }

.. code-block:: python
    :caption: Python

    # Activate the DRM and resume the existing session if any
    drm_manager.activate(True)

    # [...]

    # Deactivate the DRM, but pause the session instead of closing it
    drm_manager.deactivate(True)
