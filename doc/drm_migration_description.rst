DRM Migration description
=========================

DRM API upgrades
----------------

From v1.x to 2.x
~~~~~~~~~~~~~~~~

.. code-block:: c
    :caption: In C

    #include "accelize/drmc.h"

    struct sUserContext user_ctx = ...; // sUserContext is any structure the User may want to create

    MeteringSessionManager* p_drm = NULL;
    DRMLibErrorCode retcode;

    // Callbacks implementation
    int read_fpga_register_callback( uint32_t register offset, uint32_t* returned data, void* user_p) {...}
    int write_fpga_register_callback( uint32_t register offset, uint32_t data to write, void* user_p) {...}
    void drm_error_callback( const char* error message, void* user_p ) {...}

    // API Methods
    printf( "%s\n", DRMLib_get_version() );

    MeteringSessionManager_alloc( &p_drm,
            "path_to_conf_file",
            "path_to_cred_file",
            read_fpga_register_callback,
            write_fpga_register_callback,
            drm_error_callback,
            &user_ctx
    );
    retcode = MeteringSessionManager_start_session( p_drm );
    retcode = MeteringSessionManager_stop_session( p_drm );
    retcode = MeteringSessionManager_pause_session( p_drm );
    retcode = MeteringSessionManager_resume_session( p_drm );
    retcode = MeteringSessionManager_auto_start_session( p_drm );
    retcode = MeteringSessionManager_dump_drm_hw_report( p_drm );
    retcode = MeteringSessionManager_free( &p_drm );


Shall be replaced by:

.. code-block:: c
    :caption: In C

    #include "accelize/drmc.h"

    struct sUserContext user_ctx = ...; // sUserContext is any structure the User may want to create

    DrmManager* p_drm = NULL;
    DRM_ErrorCode retcode;
    bool resume_pause_flag = false; // true=enable pause/resume mode, false=disable pause/resume flag
    char* json_string = nullptr;

    // Callbacks implementation
    int read_fpga_register_callback( uint32_t register offset, uint32_t* returned data, void* user_p) {...}
    int write_fpga_register_callback( uint32_t register offset, uint32_t data to write, void* user_p) {...}
    void drm_error_callback( const char* error message, void* user_p ) {...}

    // API Methods
    printf( "%s\n", DrmManager_getApiVersion() );

    DRM_ErrorCode DrmManager_alloc( &p_drm,
            "path_to_conf_file",
            "path_to_cred_file",
            read_fpga_register_callback,
            write_fpga_register_callback,
            drm_error_callback,
            &user_ctx
    );
    retcode = DrmManager_activate( p_drm, resume_pause_flag );
    retcode = DrmManager_deactivate( p_drm, resume_pause_flag );
    retcode = DrmManager_get_json_string( p_drm, "{\"hw_report\":\"\"}", json_string );
    retcode = DrmManager_free( &p_drm );

    delete json_string;


.. code-block:: c++
    :caption: In C++

    #include <iostream>
    #include <string>
    #include "accelize/drm.h"

    namespace cpp = Accelize::DRMLib;

    struct sUserContext user_ctx = ...; // sUserContext is any structure the User may want to create

    // Callbacks implementation
    int read_fpga_register_callback( uint32_t register offset, uint32_t* returned data, void* user_p) {...}
    int write_fpga_register_callback( uint32_t register offset, uint32_t data to write, void* user_p) {...}
    void drm_error_callback( const char* error message, void* user_p ) {...}

    // API Methods
    try {
        std::cout << cpp::getVersion() << std::endl;

        cpp::MeteringSessionManager* p_drm = new cpp::MeteringSessionManager(
                "path_to_conf_file",
                "path_to_cred_file",
                [&]( uint32_t offset, uint32_t* value ) { return read_fpga_register_callback( offset, value, &user_ctx ); },
                [&]( uint32_t offset, uint32_t value ) { return write_fpga_register_callback( offset, value, &user_ctx ); },
                [&]( const std::string& msg ) { drm_error_callback( msg.c_str(), &user_ctx ); }
        );
        p_drm->start_session();
        p_drm->auto_start_session();
        p_drm->resume_session();
        p_drm->pause_session();
        p_drm->stop_session();
        p_drm->dump_drm_hw_report( std::cout );

    } catch( const cpp:Exception& e ) {
        std::cout << e.what() << std::endl;
    }


Shall be replaced by:

.. code-block:: c++
    :caption: In C++

    #include <iostream>
    #include <string>
    #include "accelize/drm.h"

    namespace cpp = Accelize::DRMLib;

    struct sUserContext user_ctx = ...; // sUserContext is any structure the User may want to create
    bool resume_pause_flag = false; // true=enable pause/resume mode, false=disable pause/resume flag
    char* json_string = nullptr;

    // Callback definition
    int read_fpga_register_callback( uint32_t register offset, uint32_t* returned data, void* user_p) {...}
    int write_fpga_register_callback( uint32_t register offset, uint32_t data to write, void* user_p) {...}
    void drm_error_callback( const char* error message, void* user_p ) {...}

    // API Methods
    try {
        std::cout << cpp::getApiVersion() << std::endl;

        cpp::DrmManager* p_drm = new cpp::DrmManager(
                "path_to_conf_file",
                "path_to_cred_file",
                [&]( uint32_t offset, uint32_t* value ) { return read_fpga_register_callback( offset, value, &user_ctx ); },
                [&]( uint32_t offset, uint32_t value ) { return write_fpga_register_callback( offset, value, &user_ctx ); },
                [&]( const std::string& msg ) { drm_error_callback( msg.c_str(), &user_ctx ); }
        );
        p_drm->activate( resume_pause_flag );
        p_drm->deactivate( resume_pause_flag );
        p_drm->get<std::string>( cpp::ParameterKey::hw_report );

    } catch( const cpp:Exception& e ) {
        std::cout << e.what() << std::endl;
    }


For more information about the API in your favorite language, refer to :doc:`drm_library_api`.


DRM HDK upgrade
---------------

From v2.x to 3.x
~~~~~~~~~~~~~~~~

- All files and signals prepended with `lgdn_` have been replaced by `drm_`.
- The DRM Controller and Activator IPs have been wrapped to expose an AXI4-Stream communication channel.

From v3.x to 4.x
~~~~~~~~~~~~~~~~

- `common` folder:

  - in the `common` folder there is now a specific source for xilinx simulator tool.

- `activator` folder:

  - DRM Activator top-level files (VHDL and Verilog) have been prefixed with top_ and have been moved
    to the `sim` et `syn` folder for the simulation and synthesis respectively.
  - `simu` folder name has been replaced by `sim` and `rtl` has been replace by `core`.
  - A DRM Controller BFM has been embedded directly in the Activator simulation model to unlock the IP without
    the need for an Internet connection to the Accelize License Web Server.
  - `drm_activator_0xVVVVLLLLNNNNVVVV_sim_pkg.(vhdl|sv)` file has been created to configure the simulation
    configuration and behavior. Parameters are detailed directly in the file.
  - A `constraints.sdc` file has been added in the `core` folder. It is required when `drm_aclk` and `ip_core_aclk`
    are different.

- `controller` folder:

  - RTL source files have been moved to a `rtl` folder and split in 3 different sub-folders: `core` contains
    the core of the IP, `sim` and `syn` contains the top level of the Controller IP in VHDL and SystemVerilog for
    the simulation and synthesis respectively. Top level files are prefixed with top_.
  - `sdaccel` and `vitis` folders has been create: they contains the scirpt and makefile to generate the kernel
    for those specific flows.

From v4.1 to 4.2
~~~~~~~~~~~~~~~~

- The file `drm_ip_activator_package_0xVVVVLLLLNNNNVVVV.vhdl` has been added and must be compiled under `drm_library`.
- The file `controller/rtl/core/cdc_bridge.sv` has been added and must be compiled under the default working folder.

From v4.2 to 6.0
~~~~~~~~~~~~~~~~

- Nothing to do.

