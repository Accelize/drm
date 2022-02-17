#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <getopt.h>

#include <xclhal2.h>

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/opencl.h>

/* 1. Include the C-API of the DRM Library*/
#include "accelize/drmc.h"
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

#define MAX_BATCH_CMD   32

#define PCI_VENDOR_ID   0x1D0F /* Amazon PCI Vendor ID */
#define PCI_DEVICE_ID   0xF000 /* PCI Device ID preassigned by Amazon for F1 applications */

/* Here we add the DRM base address */
#define DRM_CTRL_ADDR           0x0000000
#define DRM_ACTR_ADDR           0x0010000
#define DRM_ACTR_ADDR_RANGE     0x10000

#define ACT_STATUS_REG_OFFSET 0x38
#define MAILBOX_REG_OFFSET    0x3C
#define INC_EVENT_REG_OFFSET  0x40


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

xclDeviceHandle boardHandler;


#define test_retcode(err_condition, str) \
    if(err_condition) { \
        printf("%s: FAILED\n", str); \
        return EXIT_FAILURE; \
    }

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
    printf("   -v, --verbosity          : Specify level of vebosity from 0 (error only) to 4 (debug),\n");
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


/*
 * load_file_to_memory
 */
uint32_t load_file_to_memory(const char *filename, char **result)
{
    uint32_t size = 0;
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
    *result = NULL;
    return -1; // -1 means file opening fail
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *result = (char *)malloc(size+1);
    if (size != fread(*result, sizeof(char), size, f))
    {
    free(*result);
    return -2; // -2 means file reading fail
    }
    fclose(f);
    (*result)[size] = 0;
    return size;
}


/** Define Three callback for the DRM Lib **/
/* Callback function for DRM library to perform a thread safe register read */
int read_register( uint32_t offset, uint32_t* p_value, void* user_p ) {
    if (xclRead(*(xclDeviceHandle*)user_p, XCL_ADDR_KERNEL_CTRL, DRM_CTRL_ADDR+offset, p_value, 4) <= 0) {
        ERROR("Unable to read from the fpga!");
        return 1;
    }
    return 0;
}

/* Callback function for DRM library to perform a thread safe register write */
int write_register( uint32_t offset, uint32_t value, void* user_p ) {
    if (xclWrite(*(xclDeviceHandle*)user_p, XCL_ADDR_KERNEL_CTRL, DRM_CTRL_ADDR+offset, &value, 4) <= 0) {
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
int generate_coin(xclDeviceHandle* pci_bar_handle, uint32_t ip_index, uint32_t coins) {
    uint32_t c;

    for(c=0; c < coins; c++) {
        if (write_register(DRM_ACTR_ADDR + INC_EVENT_REG_OFFSET + ip_index * DRM_ACTR_ADDR_RANGE, 0, pci_bar_handle)) {
            ERROR("Failed to increment event counter on activator #%u.", ip_index);
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
        \"custom_field\": null }";

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
int get_activators_status( DrmManager* pDrmManager, xclDeviceHandle* pci_bar_handle, bool* activated ) {
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
        if (read_register(DRM_ACTR_ADDR + ACT_STATUS_REG_OFFSET + i * DRM_ACTR_ADDR_RANGE, &value, pci_bar_handle)) {
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


void print_activators_status( DrmManager* pDrmManager, xclDeviceHandle* pci_bar_handle ) {
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
    char * metered_data = new char[1024];
    if ( DrmManager_get_string(pDrmManager, DRM__metered_data, &metered_data) )
        ERROR("Failed to get the current metering data from FPGA design: %s", pDrmManager->error_message);
    else
        INFO(COLOR_GREEN "Current metering data fromFPGA design: %s", metered_data);
    delete[] metered_data;
}

int test_custom_field( DrmManager* pDrmManager, uint32_t value ) {
    uint32_t rd_value = 0;
    // Write value
    if ( DrmManager_set_uint(pDrmManager, DRM__custom_field, value) ) {
        ERROR("Failed to set the custom field in FPGA design: %s", pDrmManager->error_message);
        return 1;
    }
    INFO(COLOR_GREEN "Wrote '%u' custom field in FPGA design", value);
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


int interactive_mode(xclDeviceHandle* pci_bar_handle, const char* credentialFile, const char* configurationFile)
{
    DrmManager *pDrmManager = NULL;
    char answer[16] = {0};
    char* ptr;
    uint32_t val;

    if (sCurrentVerbosity < LOG_INFO)
        sCurrentVerbosity = LOG_INFO;

    /* Allocate a DrmManager, providing our previously defined callbacks */
    if (DRM_OK != DrmManager_alloc(&pDrmManager,
            configurationFile, credentialFile,
            read_register, write_register, print_drm_error,
            pci_bar_handle )) {
        ERROR("Error allocating DRM Manager object: %s", pDrmManager->error_message);
        return -1;
    }

    print_interactive_menu();
    while (strcmp(answer, "q") != 0) {

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
            if (!DrmManager_activate(pDrmManager, false))
                INFO(COLOR_CYAN "Session started");
        }

        else if (answer[0] == 'r') {
            if (!DrmManager_activate(pDrmManager, true))
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
    }

    /* Stop session and free the DrmManager object */
    if (DrmManager_free(&pDrmManager))
        ERROR("Failed to free DRM manager object: %s", pDrmManager->error_message);

    return 0;
}



int batch_mode(xclDeviceHandle* pci_bar_handle, const char* credentialFile, const char* configurationFile, const t_BatchCmd* batch, uint32_t batchSize )
{
    int ret = -1;
    DrmManager *pDrmManager = NULL;
    uint32_t val, expVal;
    bool state;
    uint32_t i;

    DEBUG("credential file is %s", credentialFile);
    DEBUG("configuration file is %s", configurationFile);

    for(i=0; i<batchSize; i++) {
        DEBUG("command #%u: name='%s', id=%u, value=%u", i, batch[i].name, batch[i].id, batch[i].value);
    }

    /* Allocate a DrmManager, providing our previously defined callbacks*/
    if (DRM_OK != DrmManager_alloc(&pDrmManager,
            configurationFile, credentialFile,
            read_register, write_register, print_drm_error,
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
                if (DRM_OK != DrmManager_activate(pDrmManager, true)) {
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
                if (DRM_OK != DrmManager_activate(pDrmManager, true)) {
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
                INFO(COLOR_CYAN "Generating coins ...");
                /* Generate coins */
                if (generate_coin(pci_bar_handle, 0, batch[i].value)) {
                    ERROR("Failed to generate coins");
                    goto batch_mode_free;
                }
                INFO("%u coins generated", batch[i].value);
                break;
            }

            case PAUSE_SESSION: {
                /* Pause the current DRM session */
                INFO(COLOR_CYAN "Pausing current session ...");
                if (DRM_OK != DrmManager_deactivate(pDrmManager, true)) {
                    ERROR("Failed to pause the DRM session: %s", pDrmManager->error_message);
                    goto batch_mode_free;
                }
                DEBUG("Pause session done");
                break;
            }

            case STOP_SESSION: {
                INFO(COLOR_CYAN "Stopping current session ...");
                /* Pause the current DRM session */
                if (DRM_OK != DrmManager_deactivate(pDrmManager, false)) {
                    ERROR("Failed to stop the DRM session: %s", pDrmManager->error_message);
                    goto batch_mode_free;
                }
                DEBUG("Stop session done");
                break;
            }

            case WAIT: {
                INFO(COLOR_CYAN "Sleeping %u seconds ...", batch[i].value);
                sleep(batch[i].value);
                DEBUG("Wake up from sleep");
                break;
            }

            case VIEW_PAGE: {
                INFO(COLOR_CYAN "Dumping DRM registry ...");
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
    char* credentialFile = DEFAULT_CREDENTIAL_FILE;
    char* configurationFile = DEFAULT_CONFIGURATION_FILE;
    static int interactive_flag = 0;
    t_BatchCmd batch_cmd[MAX_BATCH_CMD];
    uint32_t batch_cmd_len = 0;
    char *batch_str = NULL;
    cl_platform_id platform_id ;
    cl_device_id devices[2] ;
    cl_device_id device_id ;
    cl_uint num_platforms;
    cl_context context;

    /* Parse program options */
    while (1) {
        int c;
        int option_index = 0;
        static struct option long_options[] = {
                {"interactive", no_argument, NULL, 'i'},
                {"cred", required_argument, NULL, 'r'},
                {"conf", required_argument, NULL, 'o'},
                {"slot", required_argument, NULL, 's'},
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

    /* Get platform/device information */
    ret = clGetPlatformIDs(1, &platform_id,  &num_platforms);

    // Connect to a compute device
    ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_ACCELERATOR, 1, devices, &num_platforms);
    printf("num_platforms = %d\n", num_platforms);

    device_id = devices[1];

    /* Create OpenCL Context */
    context = clCreateContext( 0, 1, &device_id, NULL, NULL, &ret);

    char *fpga_bin;
    size_t fpga_bin_size;
    fpga_bin_size = load_file_to_memory(argv[1], &fpga_bin);
    test_retcode((int32_t)fpga_bin_size<0, "load kernel from xclbin");

    /* Program Device */
    clCreateProgramWithBinary(context, 1,
                            (const cl_device_id* ) &device_id, &fpga_bin_size,
                            (const unsigned char**) &fpga_bin, NULL, &ret);
    test_retcode(ret!=CL_SUCCESS, "program the FPGA from xclbin");

    // Init xclhal2 library
    if(xclProbe() < 1) {
        printf("[ERROR] xclProbe failed ...\n");
        return -1;
    }
    boardHandler = xclOpen(1, "xclhal2_logfile.log", XCL_ERROR);
    if(boardHandler == NULL) {
        printf("[ERROR] xclOpen failed ...\n");
        return -1;
    }
    if (interactive_flag) {
        ret = interactive_mode(&boardHandler, credentialFile, configurationFile);
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
        ret = batch_mode( &boardHandler, credentialFile, configurationFile, batch_cmd, batch_cmd_len );
    }

    return ret;
}
