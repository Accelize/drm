#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <stdarg.h>


/*  C Accelize DRM library utility. Intended to debug and testing. */

#include <fpga_pci.h>
#include <fpga_mgmt.h>

/* 1. Include the C-API of the DRM Library*/
#include "accelize/drmc.h"
//#include "../include/accelize/drmc/metering.h"
//#include "../include/accelize/drmc.h"
/* end of 1 */


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


#define DRM_NB_PAGES    6
#define MAX_BATCH_CMD   32

#define DRM_STATUS_REG_OFFSET

#define PCI_VENDOR_ID   0x1D0F /* Amazon PCI Vendor ID */
#define PCI_DEVICE_ID   0xF000 /* PCI Device ID preassigned by Amazon for F1 applications */

/* Here we add the DRM controller base address */
#define DRM_CTRL_BASE_ADDR      0x00000
#define ACTIVATOR_0_BASE_ADDR   0x10000
#define ACTIVATOR_RANGE_ADDR    0x10000


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
    char         name[32];
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
    printf(" 'vN': print drm registers on page N,\n");
    printf(" 'z' : print drm hw report,\n");
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


int tokenize(char str[], t_BatchCmd tokens[], uint32_t* tokens_len)
{
    char *str1, *token, *subtoken;
    char *saveptr1, *saveptr2;
    char delim1[] = ",";
    char delim2[] = "=";
    char cmd[32];
    int i;

    for (i = 0, str1 = str; ; i++, str1 = NULL) {
        token = strtok_r(str1, delim1, &saveptr1);
        if (token == NULL)
            break;

        /* Extract batch command item */
        strcpy(cmd, token);
        subtoken = strtok_r(token, delim2, &saveptr2);
        if ( (strcmp(token,"a") == 0) || (strcmp(token,"activate") == 0) ) {
            strcpy(tokens[i].name, "START");
            tokens[i].id = START_SESSION;
        } else if ( (strcmp(token,"r") == 0) || (strcmp(token,"resume") == 0) ) {
            strcpy(tokens[i].name, "RESUME");
            tokens[i].id = RESUME_SESSION;
        } else if ( (strcmp(token,"p") == 0) || (strcmp(token,"pause") == 0) ) {
            strcpy(tokens[i].name, "PAUSE");
            tokens[i].id = PAUSE_SESSION;
        } else if ( (strcmp(token,"d") == 0) || (strcmp(token,"deactivate") == 0) ) {
            strcpy(tokens[i].name, "STOP");
            tokens[i].id = STOP_SESSION;
        } else if ( (strcmp(token,"g") == 0) || (strcmp(token,"generate") == 0) ) {
            strcpy(tokens[i].name, "GENERATE");
            tokens[i].id = GENERATE_COIN;
        } else if ( (strcmp(token,"n") == 0) || (strcmp(token,"actnum") == 0) ) {
            strcpy(tokens[i].name, "NUMBER OF ACTIVATORS");
            tokens[i].id = NB_ACTIVATORS;
        } else if ( (strcmp(token,"t") == 0) || (strcmp(token,"actsta") == 0) ) {
            strcpy(tokens[i].name, "ACTIVATORS STATUS");
            tokens[i].id = ACTIVATORS_STATUS;
        } else if ( (strcmp(token,"v") == 0) || (strcmp(token,"view") == 0) ) {
            strcpy(tokens[i].name, "VIEW PAGE");
            tokens[i].id = VIEW_PAGE;
        } else if ( (strcmp(token,"w") == 0) || (strcmp(token,"wait") == 0) ) {
            strcpy(tokens[i].name, "WAIT");
            tokens[i].id = WAIT;
        } else {
            ERROR("Unsupported batch command: %s", cmd);
            return -1;
        }
        /* Extract batch command value if any */
        subtoken = strtok_r(NULL, delim2, &saveptr2);
        if (subtoken != NULL)
            tokens[i].value = atoi(subtoken);
        else
            tokens[i].value = 0;
    }
    *tokens_len = i;
    return 0;
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
void print_drm_error( const char* errmsg, void* user_p ){
    ERROR("From async callback: %s", errmsg);
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
    char* info_out = NULL;
    char* info_in = "{\
        \"license_type\": null,\
        \"num_activators\": null,\
        \"session_id\": null,\
        \"metered_data\": null,\
        \"nodelocked_request_file\": null,\
        \"custom_field\": null,\
        \"strerror\": null }";

    if (DrmManager_get_json_string(pDrmManager, info_in, &info_out )) {
        ERROR("Failed to get all DRM information: %s", pDrmManager->error_message);
        return 1;
    }
    INFO("DRM information:\n%s", info_out);
    free(info_out);
    return 0;
}


int get_num_activators( DrmManager* pDrmManager, uint32_t* p_numActivator ) {
    if (DrmManager_get_uint(pDrmManager, DRM__num_activators, p_numActivator )) {
        ERROR("Failed to get the number of activators in FPGA design: %s", pDrmManager->error_message);
        return 1;
    }
    return 0;
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
    char* license_type;
    if ( DrmManager_get_string(pDrmManager, DRM__license_type, &license_type) ) {
        ERROR("Failed to get the license type: %s", pDrmManager->error_message);
        return;
    }
    INFO(COLOR_GREEN "License type: %s", license_type);
    free(license_type);
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
    char* sessionID;
    if ( DrmManager_get_string(pDrmManager, DRM__session_id, &sessionID) ) {
        ERROR("Failed to get the current session ID in FPGA design: %s", pDrmManager->error_message);
        return;
    }
    INFO(COLOR_GREEN "Current session ID in FPGA design: %s", sessionID);
    free(sessionID);
}


void print_metered_data( DrmManager* pDrmManager ) {
    uint64_t metered_data;
    if ( DrmManager_get_uint64(pDrmManager, DRM__metered_data, &metered_data) )
        ERROR("Failed to get the current metering data from FPGA design: %s", pDrmManager->error_message);
    else
        INFO(COLOR_GREEN "Current metering data fromFPGA design: %llu", metered_data);
}

int test_custom_field( DrmManager* pDrmManager, uint32_t value ) {
    uint32_t rd_value = 0;
    // Write value
    if ( DrmManager_set_uint(pDrmManager, DRM__custom_field, value) ) {
        ERROR("Failed to set the custom field in FPGA design: %s", pDrmManager->error_message);
        return 1;
    }
    INFO(COLOR_GREEN "Wrote '%u' custom field in FPGA design with", value);
    // Read value back
    if ( DrmManager_get_uint(pDrmManager, DRM__custom_field, &rd_value) ) {
        ERROR("Failed to get the custom field in FPGA design: %s", pDrmManager->error_message);
        return 2;
    }
    INFO(COLOR_GREEN "Read custom field in FPGA design: %u", rd_value);
    // Check coherency
    if ( rd_value != value ) {
        ERROR("Value mismatch on custom field: read %u, but wrote %u", rd_value, value);
        return 3;
    }
    return 0;
}

#define DRM_CHECK(drm) if (strlen(drm.error_message) ERROR(drm.error_message);


#define DRM_RETRY(__expr, __no_retry) \
({DRM_ErrorCode __err; \
    do { \
        __err = __expr; \
        if (__err != DRM_WSMayRetry || __no_retry) break;\
        WARN("DRM operation failed but will retry in 1 second...\n"); \
        fflush(stdout); \
        sleep(1); \
    } while(1); \
__err;})


/*
* check if the corresponding AFI for hello_world is loaded
*/
int check_afi_ready(int slot_id)
{
    int ret=0;
    struct fpga_mgmt_image_info info = {0};

    /* get local image description, contains status, vendor id, and device id. */
    if (fpga_mgmt_describe_local_image(slot_id, &info,0)) {
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
    char* dump;
    uint32_t nbPageMax = DRM__page_mailbox - DRM__page_ctrlreg + 1;
    if (page > nbPageMax) {
        ERROR("Page index overflow: must be less or equal to %d", nbPageMax-1);
        return 1;
    }
    /* Print registers in page */
    if (DrmManager_get_string(pDrmManager, DRM__page_ctrlreg+page, &dump))
        ERROR("Failed to print HW page %u registry: %s", page, pDrmManager->error_message);
    printf("Page %d:\n%s", page, dump);
    free(dump);
    return 0;
}


int print_drm_report(DrmManager* pDrmManager)
{
    char* dump;
    /* Print hw report */
    if (DrmManager_get_string(pDrmManager, DRM__hw_report, &dump))
        ERROR("Failed to print HW report: %s", pDrmManager->error_message);
    printf("HW report:\n%s", dump);
    free(dump);
    return 0;
}


int interactive_mode(pci_bar_handle_t* pci_bar_handle, const char* credentialFile, const char* configurationFile, int no_retry_flag)
{
    int ret;
    DrmManager *pDrmManager = NULL;
    char answer[16] = {0};
    char* ptr;
    uint32_t val;

    if (sCurrentVerbosity < LOG_INFO)
        sCurrentVerbosity = LOG_INFO;

    /* Allocate a DrmManager, providing our previously defined callbacks */
    if (DRM_OK != DrmManager_alloc(&pDrmManager,
            configurationFile, credentialFile,
            read_drm_reg32, write_drm_reg32, print_drm_error,
            pci_bar_handle )) {
        ERROR("Error allocating DRM Manager object: %s", pDrmManager->error_message);
        return -1;
    }

    print_interactive_menu();
    while (strcmp(answer, "q") != 0) {
        ret = 0;

        printf("\nEnter your command ('h' or '?' for help): \n");
        scanf("%s" , answer) ;

        if ( (answer[0] == 'h') || (answer[0] == '?')) {
            print_interactive_menu();
        }

        else if (answer[0] == 'z') {
            if (print_drm_report(pDrmManager) == 0)
                INFO(COLOR_CYAN "HW report printed");
        }

        else if (answer[0] == 'v') {
            if (strlen(answer) < 2) {
                print_interactive_menu();
                continue;
            }
            ptr = NULL;
            val = strtol(answer+1, &ptr, 10);
            if (print_drm_page(pDrmManager, val) == 0)
                INFO(COLOR_CYAN "Registers on page %u printed", val);
        }

        else if (answer[0] == 'a') {
            if (!DRM_RETRY( DrmManager_activate(pDrmManager, false), no_retry_flag ))
                INFO(COLOR_CYAN "Session started");
        }

        else if (answer[0] == 'r') {
            if (!DRM_RETRY( DrmManager_activate(pDrmManager, true), no_retry_flag ))
                INFO(COLOR_CYAN "Session resumed");
        }

        else if (answer[0] == 'p') {
            if (!DrmManager_deactivate(pDrmManager, true))
                INFO(COLOR_CYAN "Session paused");
        }

        else if (answer[0] == 'd') {
            if (!DrmManager_deactivate(pDrmManager, false))
                INFO(COLOR_CYAN "Session stopped");
        }

        else if (answer[0] == 'g') {
            if (strlen(answer) < 2) {
                print_interactive_menu();
                continue;
            }
            ptr = NULL;
            val = strtol(answer+1, &ptr, 10);
            if (!generate_coin(pci_bar_handle, 0, val))
                INFO(COLOR_CYAN "%u coins generated", val);
        }

        else if (answer[0] == 'i') {
            print_license_type( pDrmManager );
            print_num_activators( pDrmManager );
            print_session_id( pDrmManager );
            print_metered_data( pDrmManager );
            test_custom_field( pDrmManager, rand() );
        }

        else if (answer[0] == 't') {
            print_all_information( pDrmManager );
        }

        else if (answer[0] == 's') {
            print_activators_status( pDrmManager, pci_bar_handle );
        }

        else if (answer[0] == 'q') {
            if (!DrmManager_deactivate( pDrmManager, false ))
                INFO(COLOR_CYAN "Stopped session if running and exit application");
        }

        else
            print_interactive_menu();

        if (ret)
            ERROR("Failed to execute last command: %u", ret);
    }

    /* Stop session and free the DrmManager object */
    if (DrmManager_free(&pDrmManager))
        ERROR("Failed to free DRM manager object: %s", pDrmManager->error_message);

    return ret;
}



int batch_mode(pci_bar_handle_t* pci_bar_handle, const char* credentialFile, const char* configurationFile, uint32_t no_retry_flag, const t_BatchCmd* batch, uint32_t batchSize )
{
    int ret = -1;
    DrmManager *pDrmManager = NULL;
    int32_t val, expVal;
    bool state;
    uint32_t i;

    DEBUG("credential file is %s", credentialFile);
    DEBUG("configuration file is %s", configurationFile);
    DEBUG("no-retry = %d", no_retry_flag);
    for(i=0; i<batchSize; i++) {
        DEBUG("command #%u: name='%s', id=%u, value=%u", i, batch[i].name, batch[i].id, batch[i].value);
    }

    /* Allocate a DrmManager, providing our previously defined callbacks*/
    if (DRM_OK != DrmManager_alloc(&pDrmManager,
            configurationFile, credentialFile,
            read_drm_reg32, write_drm_reg32, print_drm_error,
            pci_bar_handle
            )) {
        ERROR("Error allocating DRM Manager object");
        return -1;
    }

    for(i = 0; i < batchSize; i++) {

        expVal = batch[i].value;

        switch(batch[i].id) {

            case START_SESSION: {
                /* Start a new session */
                INFO(COLOR_CYAN
                             "Starting a new session ...");
                if (DRM_OK != DRM_RETRY(DrmManager_activate(pDrmManager, true), no_retry_flag)) {
                    ERROR("Failed to start a new session: %s", pDrmManager->error_message);
                    goto batch_mode_free;
                }
                DEBUG("Start session done");
                break;
            }

            case RESUME_SESSION: {
                /* Resume an existing session */
                INFO(COLOR_CYAN
                             "Resuming an existing session ...");
                if (DRM_OK != DRM_RETRY(DrmManager_activate(pDrmManager, true), no_retry_flag)) {
                    ERROR("Failed to resume existing session: %s", pDrmManager->error_message);
                    goto batch_mode_free;
                }
                DEBUG("Resume session done");
                break;
            }

            case NB_ACTIVATORS: {
                INFO(COLOR_CYAN "Searching for activators ...");
                if (get_num_activators(pDrmManager, &val))
                    goto batch_mode_free;
                INFO(COLOR_GREEN "Num of activators in FPGA design: %u", val );
                if (expVal == -1) {
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
                if (expVal == -1) {
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
                INFO(COLOR_CYAN
                             "Generating coins ...");
                /* Generate coins */
                if (generate_coin(pci_bar_handle, 0, batch[i].value)) {
                    ERROR("Failed to generate coins");
                    goto batch_mode_free;
                }
                INFO("%u coins generated", batch[i].value);
                break;
            }

            case PAUSE_SESSION: {
                INFO(COLOR_CYAN
                             "Pausing current session ...");
                /* Pause the current DRM session */
                if (DRM_OK != DrmManager_deactivate(pDrmManager, true)) {
                    ERROR("Failed to pause the DRM session: %s", pDrmManager->error_message);
                    goto batch_mode_free;
                }
                DEBUG("Pause session done");
                break;
            }

            case STOP_SESSION: {
                INFO(COLOR_CYAN
                             "Stopping current session ...");
                /* Pause the current DRM session */
                if (DRM_OK != DrmManager_deactivate(pDrmManager, false)) {
                    ERROR("Failed to stop the DRM session: %s", pDrmManager->error_message);
                    goto batch_mode_free;
                }
                DEBUG("Stop session done");
                break;
            }

            case WAIT: {
                INFO(COLOR_CYAN
                             "Sleeping %u seconds ...", batch[i].value);
                sleep(batch[i].value);
                DEBUG("Wake up from sleep");
                break;
            }

            case VIEW_PAGE: {
                INFO(COLOR_CYAN
                             "Dumping DRM registry ...");
                /* Diplay page N of DRM controller regsiter map */
                val = batch[i].value;
                if (print_drm_page(pDrmManager, val)) {
                    ERROR("Failed to read page %u of DRM Controller registery", val);
                    goto batch_mode_free;
                }
                DEBUG("Displayed page %u of DRM Controller registery", val);
                break;
            }

            default: {
                ERROR("Unsupported batch command ID: %u", batch[i].id);
                break;
            }
        }
        printf("\n");
    }

    ret = 0;

batch_mode_free:
    /* Free the DrmManager*/
    if (DrmManager_free(&pDrmManager))
        ERROR("Failed to free DRM manager object: %s", pDrmManager->error_message);

    return ret;
}


int main(int argc, char **argv) {

    int ret = -1;
    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
    char* credentialFile = DEFAULT_CREDENTIAL_FILE;
    char* configurationFile = DEFAULT_CONFIGURATION_FILE;
    static int interactive_flag = 0;
    static int noretry_flag = 0;
    t_BatchCmd batch_cmd[MAX_BATCH_CMD];
    uint32_t batch_cmd_len = 0;
    int slotID = 0;
    char *batch_str = NULL;

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
            case 'b': batch_str = optarg; break;
            case 'v': sCurrentVerbosity = (t_LogLevel)atoi(optarg); break;
            case 'h': print_usage(); exit(0); break;
            default:
                printf("Unknown option -%c\n", c);
                print_usage();
                abort();
        }
    }

    INFO("Using DRM Lib API version: %s", DrmManager_getApiVersion());

    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
    if(fpga_pci_init()) {
        ERROR("Unable to initialize the fpga_pci library");
        return -1;
    }

    if (check_afi_ready(slotID)) {
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
        if (batch_str == NULL) {
            ERROR("In batch mode, the -b option shall be defined with a set of commands.");
            print_usage();
            return -1;
        }
        if ( tokenize(batch_str, batch_cmd, &batch_cmd_len) ) {
            print_usage();
            return -1;
        }
        ret = batch_mode( &pci_bar_handle, credentialFile, configurationFile, noretry_flag, batch_cmd, batch_cmd_len );
    }

    return ret;
}
