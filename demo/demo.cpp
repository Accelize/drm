#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <stdarg.h>


/*  This demo demonstrate an accelerator that is a DRM protected Loopback
This design contains a data_in stream port and data_out stream port
Any data written in data_in port will be streamed out on data_out
The "loopback IP" on the FPGA is protected by a DRM controller
The loopback function will only work once a proper DRM session has been
started on the DRM controller.
This file will demonstrate how to do this with the Accelize DRM Library

This demo demonstrate the C-API of the DRM Library but note that a C++ API also exists

Find the required steps about DRM Library in comments:
1. Include the C-API of the DRM Library
2. Define Three callback for the DRM Library
3. Allocate a DrmManager, providing our previously defined callbacks
4. Auto start a DRM session, this will automatically choose between a start or resume session
5. At the end of operation either stop or pause session
6. Free the DrmManager

*/

/* This header is a tier library that operates the loopback design
It also provides the basic functions to access the AXI4-Lite bus the DRM controller is mapped onto
Important Note : Those basic functions (read and write 32 bit registers) will be used in separate thread
launched by the DRM Library, therefore they must be safe to be used in an other thread.
In our case qploopbackc fills this requirement. */

#include <sstream>

#include <fpga_pci.h>
#include <fpga_mgmt.h>

/* 1. Include the C-API of the DRM Library*/
#include "accelize/drm.h"
//#include "../include/accelize/drmc/metering.h"
//#include "../include/accelize/drmc.h"
/* end of 1 */

using namespace Accelize::DRM;


#define DEFAULT_CREDENTIAL_FILE     "../cred.json"
#define DEFAULT_CONFIGURATION_FILE  "../conf.json"


#define COLOR_RED     "\x1b[1;31m"
#define COLOR_GREEN   "\x1b[1;32m"
#define COLOR_YELLOW  "\x1b[1;33m"
#define COLOR_BLUE    "\x1b[1;34m"
#define COLOR_MAGENTA "\x1b[0;35m"
#define COLOR_CYAN    "\x1b[1;36m"
#define COLOR_RESET   "\x1b[0m"

/* Remove path from filename */
#ifdef _WIN32
#define __SHORT_FILE__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

/* Main log macro */
#define __LOG__(loglevel, color, format, ...) printf("%s%-5s, %s, %s:%d - " format "\x1b[0m\n", color, loglevel, getFormattedTime(), __SHORT_FILE__, __LINE__, ##__VA_ARGS__)

/* Specific log macros with */
#define DEBUG(format, ...) do { if (sCurrentVerbosity >= LOG_DEBUG) __LOG__("DEBUG", COLOR_MAGENTA, format, ##__VA_ARGS__); } while(0)
#define INFO(format, ...)  do { if (sCurrentVerbosity >= LOG_INFO ) __LOG__("INFO" , COLOR_RESET  , format, ##__VA_ARGS__); } while(0)
#define WARN(format, ...)  do { if (sCurrentVerbosity >= LOG_WARN ) __LOG__("WARN" , COLOR_YELLOW , format, ##__VA_ARGS__); } while(0)
#define ERROR(format, ...) do { if (sCurrentVerbosity >= LOG_ERROR) __LOG__("ERROR", COLOR_RED    , format, ##__VA_ARGS__); } while(0)


#define DRM_STATUS_REG_OFFSET

#define PCI_VENDOR_ID   0x1D0F /* Amazon PCI Vendor ID */
#define PCI_DEVICE_ID   0xF000 /* PCI Device ID preassigned by Amazon for F1 applications */

/* Here we add the DRM controller base address */
#define DRM_CTRL_BASE_ADDR      0x00000
#define ACTIVATOR_0_BASE_ADDR   0x10000
#define ACTIVATOR_RANGE_ADDR    0x10000


#define TRY try {

#define CATCH(err_msg) } catch(const std::exception& e) { \
    ERROR( "In %s, %s: %s", __FUNCTION__, err_msg, e.what() ); \
}

#define CATCH_EXIT(err_msg, err_code) } catch(const std::exception& e) { \
    ERROR( "In %s, %s: %s", __FUNCTION__, err_msg, e.what() ); \
    return err_code; \
}

#define TRY_RET int ret = -1; \
    TRY

#define CATCH_RET(err_msg, err_code) } catch(const std::exception& e) { \
    ERROR( "In %s, %s: %s", __FUNCTION__, err_msg, e.what() ); \
    ret = err_code; \
}



typedef enum {LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG} t_LogLevel;

typedef enum {
    START_SESSION,
    RESUME_SESSION,
    STOP_SESSION,
    PAUSE_SESSION,
    GENERATE_COIN,
    NB_ACTIVATORS,
    ACTIVATORS_STATUS,
    VIEW_PAGE,
    WAIT
} t_BatchCmdId;

typedef struct {
    std::string  name;
    t_BatchCmdId id;
    uint32_t     value;
} t_BatchCmd;


static t_LogLevel sCurrentVerbosity = LOG_INFO;

static const int sPfID = FPGA_APP_PF;
static const int sBarID = APP_PF_BAR0;


/* Returns the local date/time formatted as 2014-03-19 11:11:52 */
char* getFormattedTime(void) {

    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    /* Must be static, otherwise won't work */
    static char _retval[20];
    strftime(_retval, sizeof(_retval), "%Y-%m-%d %H:%M:%S", timeinfo);

    return _retval;
}


void print_usage()
{
    printf("\nUsage : \n");
    printf("   --help, -h               : Print this message\n");
    printf("   -v, --verbosity          : Sepcify level of vebosity from 0 (error only) to 4 (debug),\n");
    printf("   --cred                   : Specify path to credential file,\n");
    printf("   --conf                   : Specify path to configuration file,\n");
    printf("   --no-retry               : Disable the retry mechanism if WebService is temporarily unavailable during the start/resume and stop operations\n");
    printf("   -i, --interactive        : Run application in interactive mode. This is mutually exclusive with -b,--batch option,\n");
    printf("   -b, --batch              : Batch mode: execute a set of commands passed in CSV format. This is mutually exclusive with -i,--interactive option\n");
    printf("   -s, --slot               : If server has multiple board, specify the slot ID of the target\n");

    printf("\nList of commands available in batch mode. List of commands are passed in CSV format:\n");
    printf("   a, activate              : Start a new session,\n");
    printf("   r, resume                : Resume an opened session,\n");
    printf("   g<N>, generate=<N>       : Specify the number <N> of coins to generate,\n");
    printf("   p, pause                 : Pause the current session, leaving it pending,\n");
    printf("   d, deactivate            : Stop the current session,\n");
    printf("   n<N>, activators=<N>     : Check the number of activators in the design equals <N>,\n");
    printf("   s<0|1>, status=<0|1>     : Check the activation status of all activators are either active <0> or inactive <1>,\n");
    printf("   v<N>, view=<N>           : View the page <N> of the DRM registers,\n");
    printf("   w<S>, wait=<S>           : Wait <S> seconds\n");
}

void print_interactive_menu()
{
    printf("\nInteractive menu:\n");
    printf(" 'vN': view drm registers on page N,\n");
    printf(" 'i' : display number of activators in design,\n");
    printf(" 's' : display activators status,\n");
    printf(" 'a' : activate session (default start),\n");
    printf(" 'p' : pause session,\n");
    printf(" 'gN': generate N coins,\n");
    printf(" 'r' : resume session,\n");
    printf(" 'd' : deactivate session (default stop),\n");
    printf(" 't' : get all paramters,\n");
    printf(" 'q' : stop session and exit\n");
}


std::vector<t_BatchCmd> tokenize( std::string str ) {
    std::vector<t_BatchCmd> result;
    char delim = ',';
    std::stringstream ss(str);
    std::string cmd, value_str;
    t_BatchCmd token;

    while (std::getline(ss, cmd, delim)) {

        if ( (cmd.rfind("a", 0) == 0) || (cmd.rfind("activate", 0) == 0) ) {
            token.name = std::string("START");
            token.id = START_SESSION;
        } else if ( (cmd.rfind("r", 0) == 0) || (cmd.rfind("resume", 0) == 0) ) {
            token.name = std::string("RESUME");
            token.id = RESUME_SESSION;
        } else if ( (cmd.rfind("p", 0) == 0) || (cmd.rfind("pause", 0) == 0) ) {
            token.name = std::string("PAUSE");
            token.id = PAUSE_SESSION;
        } else if ( (cmd.rfind("d", 0) == 0) || (cmd.rfind("deactivate", 0) == 0) ) {
            token.name = std::string("STOP");
            token.id = STOP_SESSION;
        } else if ( (cmd.rfind("g", 0) == 0) || (cmd.rfind("generate", 0) == 0) ) {
            token.name = std::string("GENERATE");
            token.id = GENERATE_COIN;
        } else if ( (cmd.rfind("n", 0) == 0) || (cmd.rfind("actnum", 0) == 0) ) {
            token.name = std::string("NUMBER OF ACTIVATORS");
            token.id = NB_ACTIVATORS;
        } else if ( (cmd.rfind("s", 0) == 0) || (cmd.rfind("actsta", 0) == 0) ) {
            token.name = std::string("STATUS OF ACTIVATORS");
            token.id = ACTIVATORS_STATUS;
        } else if ( (cmd.rfind("v", 0) == 0) || (cmd.rfind("view", 0) == 0) ) {
            token.name = std::string("VIEW PAGE");
            token.id = VIEW_PAGE;
        } else if ( (cmd.rfind("w", 0) == 0) || (cmd.rfind("wait", 0) == 0) ) {
            token.name = std::string("WAIT");
            token.id = WAIT;
        } else {
            ERROR("Unsupported batch command: %s", cmd.c_str());
            throw std::runtime_error("Unsupported batch command:" + cmd);
        }
        /* Extract batch command value if any */
        value_str = cmd.substr( cmd.find("=")+1);
        if (value_str.size()) {
            std::stringstream ss;
            ss << value_str;
            ss >> token.value;
        } else {
            token.value = 0;
        }
        result.push_back(token);
    }

    return result;
}

/** Define Three callback for the DRM Lib **/
/* Callback function for DRM library to perform a thread safe register read */
int read_drm_reg32( uint32_t offset, uint32_t* p_value, void* user_p ) {
    if (fpga_pci_peek(*(pci_bar_handle_t*)user_p, DRM_CTRL_BASE_ADDR+offset, p_value)) {
        ERROR("Unable to read from the fpga!");
        return 1;
    }
    return 0;
}

/* Callback function for DRM library to perform a thread safe register write */
int write_drm_reg32( uint32_t offset, uint32_t value, void* user_p ) {
    if (fpga_pci_poke(*(pci_bar_handle_t*)user_p, DRM_CTRL_BASE_ADDR+offset, value)) {
        ERROR("Unable to write to the fpga.");
        return 1;
    }
    return 0;
}

/* Callback function for DRM library in case of asynchronous error during operation */
void print_drm_error( const std::string errmsg, void* /*user_p*/ ) {
    ERROR("%s", errmsg.c_str());
}


/** Thread functions that push and pull random data to/from FPGA streams **/
int generate_coin(pci_bar_handle_t* pci_bar_handle, uint32_t ip_index, uint32_t coins) {
    uint32_t value;
    uint32_t c;

    for(c=0; c < coins; c++) {
        if (read_drm_reg32(ACTIVATOR_0_BASE_ADDR + ip_index * ACTIVATOR_RANGE_ADDR, &value, pci_bar_handle)) {
            ERROR("Failed to read register offset 0x%X of activator #%u.", ACTIVATOR_0_BASE_ADDR, ip_index);
            return -1;
        }
    }
    return 0;
}


int print_all_information( DrmManager* pDrmManager ) {
    std::string info_str = "{\
        \"license_type\": null,\
        \"num_activators\": null,\
        \"session_id\": null,\
        \"metering_data\": null,\
        \"nodelocked_request_file\": null,\
        \"page_ctrlreg\": null,\
        \"page_vlnvfile\": null,\
        \"page_licfile\": null,\
        \"page_tracefile\": null,\
        \"page_meteringfile\": null,\
        \"page_mailbox\": null,\
        \"hw_report\": null,\
        \"custom_field\": null,\
        \"strerror\": null }";

    TRY
        pDrmManager->get( info_str );
        INFO("DRM information:\n%s", info_str.c_str());
        return 0;
    CATCH_EXIT("Failed to get all DRM information", 1)
}


int get_num_activators( DrmManager* pDrmManager, uint32_t* p_numActivator ) {
    TRY
        *p_numActivator = pDrmManager->get<uint32_t>( ParameterKey::num_activators );
        return 0;
    CATCH_EXIT( "Failed to get the number of activators in FPGA design", 1 )
}


/** get activation status of all activators */
int get_activators_status( DrmManager* pDrmManager, pci_bar_handle_t* pci_bar_handle, bool* activated ) {
    uint32_t value=0;
    uint32_t code_rdy;
    uint32_t nb_activators;
    uint32_t i;
    bool active, all_active;

    // Get the number of activators in the HW
    if ( get_num_activators(pDrmManager, &nb_activators) )
        return 1;

    all_active = true;
    for(i=0; i<nb_activators; i++) {
        if (read_drm_reg32(ACTIVATOR_0_BASE_ADDR + i * ACTIVATOR_RANGE_ADDR, &value, pci_bar_handle)) {
            ERROR("Failed to read register of activator #%u", i);
            return 2;
        }
        code_rdy = (value >> 1) & 1;
        active = value & 1;
        DEBUG("Succeeded to read register of activator #%u: 0x%08X (ready=%d, active=%d)", i, value, code_rdy, active);
        if (active)
            INFO("Status of activator #%u: " COLOR_GREEN "ACTIVATED", i);
        else
            INFO("Status of activator #%u: " COLOR_RED "LOCKED", i);
        all_active &= active;
    }
    *activated = all_active;
    return 0;
}


void print_license_type( DrmManager* pDrmManager ) {
    TRY
        std::string license_type = pDrmManager->get<std::string>( ParameterKey::license_type );
        INFO(COLOR_GREEN "License type: %s", license_type.c_str());
    CATCH("Failed to get the license type")
}


void print_num_activators( DrmManager* pDrmManager ) {
    uint32_t numActivator = 0;
    if ( get_num_activators( pDrmManager, &numActivator ) == 0 )
        INFO(COLOR_GREEN "Num of activators in FPGA design: %u", numActivator );
}


void print_activators_status( DrmManager* pDrmManager, pci_bar_handle_t* pci_bar_handle ) {
    bool active = false;
    get_activators_status( pDrmManager, pci_bar_handle, &active );
}


void print_session_id( DrmManager* pDrmManager ) {
    TRY
        std::string sessionID = pDrmManager->get<std::string>( ParameterKey::session_id);
        INFO(COLOR_GREEN "Current session ID in FPGA design: %s", sessionID.c_str());
    CATCH("Failed to get the current session ID in FPGA design")
}


void print_metering_data( DrmManager* pDrmManager ) {
    TRY
        uint64_t metering_data = pDrmManager->get<uint64_t>( ParameterKey::metering_data );
        INFO(COLOR_GREEN "Current metering data fromFPGA design: %llu", metering_data);
    CATCH("Failed to get the current metering data from FPGA design")
}

int test_custom_field( DrmManager* pDrmManager, uint32_t value ) {
    // Write value
    TRY
        pDrmManager->set( ParameterKey::custom_field, value);
        INFO(COLOR_GREEN "Wrote '%u' custom field in FPGA design with", value);
    CATCH_EXIT("Failed to set the custom field in FPGA design", 1)

    // Read value back
    uint32_t rd_value = 0;
    TRY
        rd_value = pDrmManager->get<uint32_t>( ParameterKey::custom_field );
        INFO(COLOR_GREEN "Read custom field in FPGA design: %u", rd_value);
    CATCH_EXIT("Failed to get the custom field in FPGA design", 2)

    // Check coherency
    if ( rd_value != value ) {
        ERROR("Value mismatch on custom field: read %u, but wrote %u", rd_value, value);
        return 3;
    }
    return 0;
}

void print_last_error( DrmManager* pDrmManager ) {
    TRY
        std::string errMsg = pDrmManager->get<std::string>( ParameterKey::strerror );
        if ( errMsg.size() == 0)
            INFO(COLOR_GREEN "No error message so far.");
        else
            INFO(COLOR_RED "Last error message: %s", errMsg.c_str());
    CATCH("Failed to get the last error message (if any)")
}


#define DRM_RETRY(__expr, __no_retry) \
    do { \
        try { \
            __expr; \
            break; \
        } catch( const Exception& e) { \
            if (e.getErrCode() != DRM_WSMayRetry || __no_retry) break; \
            WARN("DRM operation failed but will retry in 1 second ...\n"); fflush(stdout); \
            sleep(1); \
        } \
    } while(1);


/*
* check if the corresponding AFI for hello_world is loaded
*/
int check_afi_ready(int slot_id)
{
    int ret=0;
    struct fpga_mgmt_image_info info;

    /* get local image description, contains status, vendor id, and device id. */
    if (fpga_mgmt_describe_local_image( slot_id, &info, 0)) {
        ERROR("Unable to get AFI information from slot %d. Are you running as root?", slot_id);
        return 1;
    }

    /* check to see if the slot is ready */
    if (info.status != FPGA_STATUS_LOADED) {
        ERROR("AFI in Slot %d is not in READY state !", slot_id);
        return 1;
    }

    INFO("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n", info.spec.map[sPfID].vendor_id, info.spec.map[sPfID].device_id);

    /* confirm that the AFI that we expect is in fact loaded */
    if (info.spec.map[sPfID].vendor_id != PCI_VENDOR_ID ||
        info.spec.map[sPfID].device_id != PCI_DEVICE_ID) {
        INFO("%s: AFI does not show expected PCI vendor id and device ID. If the AFI "
             "was just loaded, it might need a rescan. Rescanning now.\n", __FUNCTION__);

        if(fpga_pci_rescan_slot_app_pfs(slot_id)) {
            ERROR("%s: Unable to update PF for slot %d",__FUNCTION__, slot_id);
            return 1;
        }

        /* get local image description, contains status, vendor id, and device id. */
        if(fpga_mgmt_describe_local_image(slot_id, &info,0)) {
            ERROR("%s: Unable to get AFI information from slot %d",__FUNCTION__, slot_id);
            return 1;
        }

        INFO("%s: AFI PCI  Vendor ID: 0x%x, Device ID 0x%x",
             __FUNCTION__,
             info.spec.map[sPfID].vendor_id,
             info.spec.map[sPfID].device_id);

        /* confirm that the AFI that we expect is in fact loaded after rescan */
        if (info.spec.map[sPfID].vendor_id != PCI_VENDOR_ID ||
            info.spec.map[sPfID].device_id != PCI_DEVICE_ID) {
            ret = 1;
            ERROR("%s: The PCI vendor id and device of the loaded AFI are not "
                  "the expected values.",__FUNCTION__);
        }
    }

    return ret;
}


int print_drm_page(DrmManager* pDrmManager, uint32_t page)
{
    uint32_t nbPageMax = ParameterKey::page_mailbox - ParameterKey::page_ctrlreg + 1;
    if (page > nbPageMax) {
        ERROR("Page index overflow: must be less or equal to %d", nbPageMax-1);
        return 1;
    }
    TRY
        /* Print registers in page */
        pDrmManager->get<std::string>( (ParameterKey)(ParameterKey::page_ctrlreg + page));
        return 0;
    CATCH_EXIT("Failed to print HW registry", 1)
}


int print_drm_report(DrmManager* pDrmManager)
{
    TRY
        std::string report;
        /* Print hw report */
        report = pDrmManager->get<std::string>( ParameterKey::hw_report );
        return 0;
    CATCH_EXIT("Failed to print HW report", 1)
}


int interactive_mode(pci_bar_handle_t* pci_bar_handle, const std::string& credentialFile, const std::string& configurationFile, const int& no_retry_flag)
{
    int ret;
    DrmManager *pDrmManager = nullptr;
    std::string answer;
    uint32_t val;
    char cmd;

    if (sCurrentVerbosity < LOG_INFO)
        sCurrentVerbosity = LOG_INFO;

    /* Allocate a DrmManager, providing our previously defined callbacks */
    pDrmManager = new DrmManager(
            configurationFile, credentialFile,
            [&]( uint32_t offset, uint32_t* p_value ) { /* Read DRM register */
                return read_drm_reg32( offset, p_value, pci_bar_handle );
            },
            [&]( uint32_t offset, uint32_t value ) {    /* Write DRM register */
                return write_drm_reg32( offset, value, pci_bar_handle );
            },
            [&]( const std::string& msg ) {
                print_drm_error( msg, nullptr );
            }
        );

    print_interactive_menu();
    while (answer.compare("q") != 0) {
        ret = 0;

        std::cout << "\nEnter your command ('h' or '?' for help)" << std::endl;
        getline(std::cin, answer);

        std::stringstream ss;
        ss << answer;
        ss >> cmd;
        if (answer.size() > 1)
            ss >> val;

        if ( (answer[0] == 'h') || (answer[0] == '?')) {
            print_interactive_menu();
        }

        else if (cmd == 'z') {
            if (print_drm_report(pDrmManager) == 0)
                INFO(COLOR_GREEN "HW report printed");
        }

        else if (cmd == 'v') {
            if (answer.size() < 2) {
                print_interactive_menu();
                continue;
            }
            if (print_drm_page(pDrmManager, val) == 0)
                INFO(COLOR_GREEN "Registers on page %u printed", val);
        }

        else if (cmd == 'a') {
            DRM_RETRY( pDrmManager->activate(false), no_retry_flag )
            INFO(COLOR_GREEN "Session started");
        }

        else if (cmd == 'r') {
            DRM_RETRY( pDrmManager->activate(true), no_retry_flag )
            INFO(COLOR_GREEN "Session resumed");
        }

        else if (cmd == 'p') {
            DRM_RETRY( pDrmManager->deactivate(true), no_retry_flag )
            INFO(COLOR_GREEN "Session paused");
        }

        else if (cmd == 'd') {
            DRM_RETRY( pDrmManager->deactivate(false), no_retry_flag )
            INFO(COLOR_GREEN "Session stopped");
        }

        else if (cmd == 'g') {
            if (answer.size() < 2) {
                print_interactive_menu();
                continue;
            }
            if (!generate_coin(pci_bar_handle, 0, val))
                INFO(COLOR_GREEN "%u coins generated", val);
        }

        else if (cmd == 'i') {
            print_license_type( pDrmManager );
            print_num_activators( pDrmManager );
            print_session_id( pDrmManager );
            print_metering_data( pDrmManager );
            test_custom_field( pDrmManager, rand() );
            print_last_error( pDrmManager );
        }

        else if (cmd == 't') {
            print_all_information(pDrmManager);
        }

        else if (cmd == 's') {
            print_activators_status( pDrmManager, pci_bar_handle );
        }

        else if (cmd == 'q') {
            DRM_RETRY(pDrmManager->deactivate(false), no_retry_flag)
            INFO(COLOR_GREEN "Stopped session if running and exit application");
        }

        else
            print_interactive_menu();

        if (ret)
            ERROR("Failed to execute last command: %u", ret);
    }

    /* Check error message if any */
    print_last_error( pDrmManager );

    /* Stop session and free the DrmManager object */
    delete pDrmManager;

    return ret;
}



int batch_mode(pci_bar_handle_t* pci_bar_handle, const std::string& credentialFile, const std::string& configurationFile, const int& no_retry_flag, const std::vector<t_BatchCmd> batch_list)
{
    int ret = -1;
    DrmManager *pDrmManager = nullptr;
    uint32_t val, expVal;
    bool state;
    uint32_t i=0;

    DEBUG("credential file is %s", credentialFile.c_str());
    DEBUG("configuration file is %s", configurationFile.c_str());
    DEBUG("no-retry = %d", no_retry_flag);
    for(const auto& cmd: batch_list) {
        DEBUG("command #%u: name='%s', id=%u, value=%u", i, cmd.name.c_str(), cmd.id, cmd.value);
        i ++;
    }

    /* Allocate a DrmManager, providing our previously defined callbacks*/
    pDrmManager = new DrmManager(
            configurationFile, credentialFile,
            [&]( uint32_t offset, uint32_t* p_value ) { /* Read DRM register */
                return read_drm_reg32( offset, p_value, pci_bar_handle );
            },
            [&]( uint32_t offset, uint32_t value ) {    /* Write DRM register */
                return write_drm_reg32( offset, value, pci_bar_handle );
            },
            [&]( const std::string& msg ) {
                print_drm_error( msg, nullptr );
            }
        );

    for(const auto batch: batch_list) {

        expVal = batch.value;

        switch(batch.id) {

            case START_SESSION: {
                /* Start a new session */
                INFO(COLOR_CYAN "Starting a new session ...");
                DRM_RETRY(pDrmManager->activate(true), no_retry_flag)
                DEBUG("Start session done");
                break;
            }

            case RESUME_SESSION: {
                /* Resume an existing session */
                INFO(COLOR_CYAN "Resuming an existing session ...");
                DRM_RETRY(pDrmManager->activate(true), no_retry_flag)
                DEBUG("Resume session done");
                break;
            }

            case NB_ACTIVATORS: {
                INFO(COLOR_CYAN "Searching for activators ...");
                if ( get_num_activators( pDrmManager, &val ) )
                    goto batch_mode_free;
                INFO(COLOR_GREEN "Num of activators in FPGA design: %u", val );
                if (expVal == (uint32_t)(-1)) {
                    /* Only display number of activators */
                    INFO("Number of activators found: %u", val);
                    WARN("No check made on the number of activators");
                    break;
                }
                /* Check number of activators */
                if (val != expVal) {
                    ERROR("Found %u activators but expect %u", val, expVal);
                    goto batch_mode_free;
                }
                INFO("Found expected number of activators: %u", val);
                break;
            }

            case ACTIVATORS_STATUS: {
                INFO(COLOR_CYAN "Collecting activators status ...");
                if (get_activators_status(pDrmManager, pci_bar_handle, &state))
                    goto batch_mode_free;
                if (expVal == (uint32_t)(-1)) {
                    WARN("No check made on the activation status");
                    break;
                }
                /* Check activation status */
                if (state != (bool) expVal) {
                    ERROR("Activator status is %u but expect %u", state, expVal);
                    goto batch_mode_free;
                }
                DEBUG("Activator status is as expected: %u", state);
                break;
            }

            case GENERATE_COIN: {
                INFO(COLOR_CYAN "Generating coins ...");
                /* Generate coins */
                if (generate_coin(pci_bar_handle, 0, batch.value)) {
                    ERROR("Failed to generate coins");
                    goto batch_mode_free;
                }
                INFO("%u coins generated", batch.value);
                break;
            }

            case PAUSE_SESSION: {
                /* Pause the current DRM session */
                INFO(COLOR_CYAN "Pausing current session ...");
                DRM_RETRY(pDrmManager->deactivate(true), no_retry_flag)
                DEBUG("Pause session done");
                break;
            }

            case STOP_SESSION: {
                /* Pause the current DRM session */
                INFO(COLOR_CYAN "Stopping current session ...");
                DRM_RETRY(pDrmManager->deactivate(false), no_retry_flag)
                DEBUG("Stop session done");
                break;
            }

            case WAIT: {
                INFO(COLOR_CYAN "Sleeping %u seconds ...", batch.value);
                sleep(batch.value);
                DEBUG("Wake up from sleep");
                break;
            }

            case VIEW_PAGE: {
                /* Diplay page N of DRM controller regsiter map */
                INFO(COLOR_CYAN "Dumping DRM registry ...");
                val = batch.value;
                if (print_drm_page(pDrmManager, val)) {
                    ERROR("Failed to read page %u of DRM Controller registery", val);
                    goto batch_mode_free;
                }
                DEBUG("Displayed page %u of DRM Controller registery", val);
                break;
            }

            default: {
                ERROR("Unsupported batch command ID: %u", batch.id);
                break;
            }
        }
        printf("\n");
    }

    ret = 0;

batch_mode_free:
    /* Free the DrmManager*/
    delete pDrmManager;

    return ret;
}


int main(int argc, char **argv) {

    int ret = -1;
    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
    std::string credentialFile( DEFAULT_CREDENTIAL_FILE );
    std::string configurationFile( DEFAULT_CONFIGURATION_FILE );
    static int interactive_flag = 0;
    static int noretry_flag = 0;
    std::vector<t_BatchCmd> batch_cmd_list;
    int slotID = 0;
    std::string batch_str;

    /* Parse program options */
    while (1) {
        int c;
        int option_index = 0;
        static struct option long_options[] = {
                {"interactive", no_argument, NULL, 'i'},
                {"cred", required_argument, NULL, 'r'},
                {"conf", required_argument, NULL, 'o'},
                {"slot", required_argument, NULL, 's'},
                {"no-retry", no_argument, &noretry_flag, 1},
                {"batch", required_argument, NULL, 'b'},
                {"verbosity", required_argument, NULL, 'v'},
                {"help", no_argument, NULL, 'h'},
                {0, 0, 0, 0 }
        };

        c = getopt_long(argc, argv, "ir:o:s:b:v:h", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 0: break;
            case 'i': interactive_flag = 1; break;
            case 'r': credentialFile = optarg; break;
            case 'o': configurationFile = optarg; break;
            case 's': slotID = atoi(optarg); break;
            case 'b': batch_str = std::string(optarg); break;
            case 'v': sCurrentVerbosity = (t_LogLevel)atoi(optarg); break;
            case 'h': print_usage(); exit(0); break;
            default:
                printf("Unknown option -%c\n", c);
                print_usage();
                abort();
        }
    }

    if (batch_str.size()) {
        batch_cmd_list = tokenize(batch_str);
        if (batch_cmd_list.empty()) {
            print_usage();
            return -1;
        }
    }

    INFO("Using DRM Lib API version: %s", getApiVersion());

    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
    if(fpga_pci_init()) {
        ERROR("Unable to initialize the fpga_pci library");
        return -1;
    }

    if(check_afi_ready(slotID)) {
        ERROR("AFI not ready");
        return -1;
    }

    if(fpga_pci_attach(slotID, sPfID, sBarID, 0, &pci_bar_handle)) {
        ERROR("Unable to attach to the AFI on slot id %d", slotID);
        return -1;
    }

    if (interactive_flag) {
        ret = interactive_mode(&pci_bar_handle, credentialFile, configurationFile, noretry_flag);
    }

    else {
        ret = batch_mode(&pci_bar_handle, credentialFile, configurationFile, noretry_flag, batch_cmd_list);
    }

    return ret;
}
