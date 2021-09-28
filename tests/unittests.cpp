/*  C++ Accelize DRM library utility. Intended to debug and testing. */

#include <iostream>
#include <getopt.h>
#include <memory>
#include <sstream>
#include <fstream>
#include <unistd.h>

/* JsonCPP Library */
#include <json/json.h>

/* AWS FPGA Management Library */
#include <fpga_pci.h>
#include <fpga_mgmt.h>

/* 1. Include the C-API of the DRM Library*/
#include "accelize/drm.h"
#include "accelize/drmc.h"
/* end of 1 */


#define PCI_VENDOR_ID   0x1D0F /* Amazon PCI Vendor ID */
#define PCI_DEVICE_ID   0xF000 /* PCI Device ID preassigned by Amazon for F1 applications */

/* Here we add the DRM controller base address */
#define DRM_CTRL_BASE_ADDR      0x00000

using namespace std;

namespace cpp = Accelize::DRM;


static string sAsyncErrorMessage;


/** Define Three callback for the DRM Lib **/
/* Callback function for DRM library to perform a thread safe register read */
static int read_drm_register( uint32_t offset, uint32_t* p_value, void* user_p ) {
    if (fpga_pci_peek(*(pci_bar_handle_t*)user_p, DRM_CTRL_BASE_ADDR+offset, p_value)) {
        cerr << "Unable to read to the fpga." << endl;
        return 1;
    }
    return 0;
}

/* Callback function for DRM library to perform a thread safe register write */
static int write_drm_register( uint32_t offset, uint32_t value, void* user_p ) {
    if (fpga_pci_poke(*(pci_bar_handle_t*)user_p, DRM_CTRL_BASE_ADDR+offset, value)) {
        cerr << "Unable to write to the fpga." << endl;
        return 1;
    }
    return 0;
}

/* Callback function for DRM library in case of asynchronous error during operation */
static void print_async_error( const char* errmsg, void* /*user_p*/ ) {
    cerr << "From async callback: " << errmsg << endl;
    sAsyncErrorMessage = errmsg + string("\n");
}


Json::Value parseJsonString( const std::string &json_string ) {
    Json::Value json_node;
    std::string parseErr;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> const reader( builder.newCharReader() );

    if ( json_string.size() == 0 )
        throw std::runtime_error( "Cannot parse an empty JSON string" );

    if ( !reader->parse( json_string.c_str(), json_string.c_str() + json_string.size(),
            &json_node, &parseErr) ) {
        std::stringstream ss;
        ss << "Cannot parse JSON string because " << parseErr << std::endl;
        throw std::runtime_error( ss.str() );
    }
    if ( json_node.empty() || json_node.isNull() )
        throw std::runtime_error( "JSON string is empty" );

    return json_node;
}


Json::Value parseJsonFile( const std::string& file_path ) {
    Json::Value json_node;
    std::string file_content;
    std::stringstream ss;

    // Open file
    std::ifstream fh( file_path );
    if ( !fh.good() ) {
        ss << "Cannot find JSON file: " << file_path << std::endl;
        throw std::runtime_error( ss.str() );
    }
    // Read file content
    fh.seekg( 0, std::ios::end );
    file_content.reserve( fh.tellg() );
    fh.seekg( 0, std::ios::beg );
    file_content.assign( (std::istreambuf_iterator<char>(fh)), std::istreambuf_iterator<char>() );
    // Parse content as a JSON object
    try {
        json_node = parseJsonString( file_content );
    } catch( const std::runtime_error& e ) {
        ss << "Cannot parse JSON file " <<  file_path << ": " << e.what() << std::endl;
        throw std::runtime_error( ss.str() );
    }
    return json_node;
}



class DrmManagerMaker {

    static const int sPfID = FPGA_APP_PF;
    static const int sBarID = APP_PF_BAR0;

    DrmManager *pDrmManager_c = nullptr;
    cpp::DrmManager *pDrmManager = nullptr;

public:
    bool mIsCpp = false;
    string mConfFilePath;
    string mCredFilePath;
    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;

private:

    /* check if the corresponding AFI for hello_world is loaded */
    static int check_afi_ready(int slot_id) {
        int ret = 0;
        struct fpga_mgmt_image_info info;

        /* get local image description, contains status, vendor id, and device id. */
        if (fpga_mgmt_describe_local_image(slot_id, &info, 0)) {
            cerr << "Unable to get AFI information from slot " << slot_id << ". ";
            cerr << "Are you running as root? " << endl;
            return 1;
        }

        /* check to see if the slot is ready */
        if (info.status != FPGA_STATUS_LOADED) {
            cerr << "AFI in Slot " << slot_id << " is not in READY state !" << endl;
            return 1;
        }

        /* confirm that the AFI that we expect is in fact loaded */
        if (info.spec.map[sPfID].vendor_id != PCI_VENDOR_ID ||
            info.spec.map[sPfID].device_id != PCI_DEVICE_ID) {

            cout << "AFI does not show expected PCI vendor id and device ID. If the AFI";
            cout << "was just loaded, it might need a rescan. Rescanning now." << endl;

            if(fpga_pci_rescan_slot_app_pfs(slot_id)) {
                cerr << "Unable to update PF for slot " << slot_id << endl;
                return 1;
            }

            /* get local image description, contains status, vendor id, and device id. */
            if(fpga_mgmt_describe_local_image(slot_id, &info,0)) {
                cerr << "Unable to get AFI information from slot " << slot_id << endl;
                return 1;
            }

            /* confirm that the AFI that we expect is in fact loaded after rescan */
            if (info.spec.map[sPfID].vendor_id != PCI_VENDOR_ID ||
                info.spec.map[sPfID].device_id != PCI_DEVICE_ID) {
                ret = 1;
                cerr << "The PCI vendor id and device of the loaded AFI ";
                cerr << "are not the expected values." << endl;
            }
        }
        return ret;
    }

#define IF_CPP \
    if (mIsCpp) { \
        try {
#define ELSE_C \
        } catch( const std::exception& e ) { \
            cout << "ERROR in " << __FUNCTION__ << ": " << e.what() << endl; \
            throw; \
        } \
    } else { \
        int ret;

#define END_IF \
        if (ret != DRM_ErrorCode::DRM_OK) { \
            cout << "ERROR in " << __FUNCTION__ << ": " << pDrmManager_c->error_message << endl; \
            throw(cpp::Exception( (DRM_ErrorCode)ret, string(pDrmManager_c->error_message) )); \
        } \
    }

public:

    DrmManagerMaker( const int& slot_id, const bool& is_cpp,
                     const string& conf_file, const string& cred_file) {
        mIsCpp = is_cpp;
        mConfFilePath = string(conf_file);
        mCredFilePath = string(cred_file);

        /* initialize the fpga_mgmt library */
        if (fpga_mgmt_init()) {
            throw runtime_error("Unable to initialize the fpga_mgmt library");
        }

        /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
        if (fpga_pci_init()) {
            throw runtime_error("Unable to initialize the fpga_pci library");
        }

        if (check_afi_ready(slot_id)) {
            throw runtime_error("AFI not ready");
        }

        if (fpga_pci_attach(slot_id, sPfID, sBarID, 0, &pci_bar_handle)) {
            throw runtime_error("Unable to attach to the AFI on slot id " + to_string(slot_id));
        }
    }

    void create() {
        IF_CPP
            pDrmManager = new cpp::DrmManager(
                    mConfFilePath, mCredFilePath,
                    [&](uint32_t offset, uint32_t *p_value) {
                        return read_drm_register(offset, p_value, &pci_bar_handle);
                    },
                    [&](uint32_t offset, uint32_t value) {
                        return write_drm_register(offset, value, &pci_bar_handle);
                    },
                    [&](const string &msg) {
                        print_async_error(msg.c_str(), nullptr);
                    }
            );
        ELSE_C
            ret = DrmManager_alloc( &pDrmManager_c, mConfFilePath.c_str(), mCredFilePath.c_str(),
                        read_drm_register, write_drm_register, print_async_error, &pci_bar_handle );
        END_IF
    }

    void destroy() {
        IF_CPP
                delete pDrmManager;
        ELSE_C
            ret = DrmManager_free( &pDrmManager_c );
        END_IF
    }

    void activate( const bool& resume_session_request = false ) {
        IF_CPP
            pDrmManager->activate( resume_session_request );
        ELSE_C
            ret = DrmManager_activate( pDrmManager_c, resume_session_request );
        END_IF

    }

    void deactivate( const bool& pause_session_request = false ) {
        IF_CPP
            pDrmManager->deactivate( pause_session_request );
        ELSE_C
            ret = DrmManager_deactivate( pDrmManager_c, pause_session_request );
        END_IF
    }

    // get function flavors

    bool get_bool( const uint32_t key ) {
        bool value;
        IF_CPP
                value = pDrmManager->get<bool>( (cpp::ParameterKey)key );
            ELSE_C
                ret = DrmManager_get_bool( pDrmManager_c, (DrmParameterKey)key, &value );
        END_IF
        return value;
    }

    int32_t get_int( const uint32_t key ) {
        int32_t value;
        IF_CPP
            value = pDrmManager->get<int32_t>( (cpp::ParameterKey)key );
        ELSE_C
            ret = DrmManager_get_int( pDrmManager_c, (DrmParameterKey)key, &value );
        END_IF
        return value;
    }

    uint32_t get_uint( const uint32_t key ) {
        uint32_t value;
        IF_CPP
            value = pDrmManager->get<int32_t>( (cpp::ParameterKey)key );
        ELSE_C
            ret = DrmManager_get_uint( pDrmManager_c, (DrmParameterKey)key, &value );
        END_IF
        return value;
    }

    int64_t get_int64( const uint32_t key ) {
        long long value;
        IF_CPP
                value = pDrmManager->get<int64_t>( (cpp::ParameterKey)key );
            ELSE_C
                ret = DrmManager_get_int64( pDrmManager_c, (DrmParameterKey)key, &value );
        END_IF
        return value;
    }

    uint64_t get_uint64( const uint32_t key ) {
        unsigned long long value;
        IF_CPP
                value = pDrmManager->get<uint64_t>( (cpp::ParameterKey)key );
            ELSE_C
                ret = DrmManager_get_uint64( pDrmManager_c, (DrmParameterKey)key, &value );
        END_IF
        return value;
    }

    float get_float( const uint32_t key ) {
        float value;
        IF_CPP
                value = pDrmManager->get<float>( (cpp::ParameterKey)key );
            ELSE_C
                ret = DrmManager_get_float( pDrmManager_c, (DrmParameterKey)key, &value );
        END_IF
        return value;
    }

    double get_double( const uint32_t key ) {
        double value;
        IF_CPP
                value = pDrmManager->get<double>( (cpp::ParameterKey)key );
            ELSE_C
                ret = DrmManager_get_double( pDrmManager_c, (DrmParameterKey)key, &value );
        END_IF
        return value;
    }

    string get_string( const uint32_t key ) {
        string value;
        IF_CPP
                value = pDrmManager->get<string>( (cpp::ParameterKey)key );
        ELSE_C
            char* val_char;
            ret = DrmManager_get_string( pDrmManager_c, (DrmParameterKey)key, &val_char );
            value = string(val_char);
        END_IF
        return value;
    }

    void get_json_string( string& json_string ) {
        IF_CPP
            pDrmManager->get( json_string );
        ELSE_C
            char *val_char = nullptr;
            ret = DrmManager_get_json_string( pDrmManager_c, json_string.c_str(), &val_char );
            if ( ret == DRM_ErrorCode::DRM_OK ) {
                json_string = string(val_char);
                delete val_char;
            }
        END_IF
    }

    void get_json_value( Json::Value& json_value ) {
        IF_CPP
                pDrmManager->get( json_value );
        ELSE_C
            char *val_char = nullptr;
            ret = DrmManager_get_json_string( pDrmManager_c, json_value.toStyledString().c_str(),
                    &val_char );
            if ( ret == DRM_ErrorCode::DRM_OK ) {
                try {
                    json_value = parseJsonString( string(val_char) );
                    /*for (auto it = json_out.begin(); it != json_out.end(); ++it) {
                        json_value[it.key()] = *it;
                        std::cout <<  << "\n";
                        std::cout << (*it)["Name"].get<std::string>() << "\n";
                        std::cout << (*it)["Last modified"].get<std::string>() << "\n";
                    }*/
                } catch(const std::runtime_error& e) {
                    snprintf(pDrmManager_c->error_message, sizeof(pDrmManager_c->error_message),
                            "%s", e.what());
                    ret = -1;
                }
            }
        END_IF
    }

    // set function flavors

    void set_bool( const uint32_t key, const bool& value ) {
        IF_CPP
            pDrmManager->set<bool>( (cpp::ParameterKey)key, value );
        ELSE_C
            ret = DrmManager_set_bool( pDrmManager_c, (DrmParameterKey)key, value );
        END_IF
    }

    void set_int( const uint32_t key, const int32_t& value ) {
        IF_CPP
            pDrmManager->set<int32_t>( (cpp::ParameterKey)key, value );
        ELSE_C
            ret = DrmManager_set_int( pDrmManager_c, (DrmParameterKey)key, value );
        END_IF
    }

    void set_uint( const uint32_t key, const uint32_t& value ) {
        IF_CPP
            pDrmManager->set<int32_t>( (cpp::ParameterKey)key, value );
        ELSE_C
            ret = DrmManager_set_uint( pDrmManager_c, (DrmParameterKey)key, value );
        END_IF
    }

    void set_int64( const uint32_t key, const int64_t& value ) {
        IF_CPP
            pDrmManager->set<int64_t>( (cpp::ParameterKey)key, value );
        ELSE_C
            ret = DrmManager_set_int64( pDrmManager_c, (DrmParameterKey)key, value );
        END_IF
    }

    void set_uint64( const uint32_t key, const uint64_t& value ) {
        IF_CPP
            pDrmManager->set<uint64_t>( (cpp::ParameterKey)key, value );
        ELSE_C
            ret = DrmManager_set_uint64( pDrmManager_c, (DrmParameterKey)key, value );
        END_IF
    }

    void set_float( const uint32_t key, const float& value ) {
        IF_CPP
            pDrmManager->set<float>( (cpp::ParameterKey)key, value );
        ELSE_C
            ret = DrmManager_set_float( pDrmManager_c, (DrmParameterKey)key, value );
        END_IF
    }

    void set_double( const uint32_t key, const double& value ) {
        IF_CPP
            pDrmManager->set<double>( (cpp::ParameterKey)key, value );
        ELSE_C
            ret = DrmManager_set_double( pDrmManager_c, (DrmParameterKey)key, value );
        END_IF
    }

    void set_string( const uint32_t key, const string& value ) {
        IF_CPP
            pDrmManager->set<string>( (cpp::ParameterKey)key, value );
        ELSE_C
            ret = DrmManager_set_string( pDrmManager_c, (DrmParameterKey)key, value.c_str() );
        END_IF
    }

    void set_json_string( const string& json_string ) {
        IF_CPP
            pDrmManager->set( json_string );
        ELSE_C
            ret = DrmManager_set_json_string( pDrmManager_c, json_string.c_str() );
        END_IF
    }

    void set_json_value( const Json::Value& json_value ) {
        IF_CPP
                pDrmManager->set( json_value );
        ELSE_C
            ret = DrmManager_set_json_string( pDrmManager_c, json_value.toStyledString().c_str() );
        END_IF
    }

};


/////////////////////////////////
/// TEST FUNCTIONS
/////////////////////////////////

static DrmManagerMaker* sDrm = nullptr;


// Test null callback pointers
int test_null_read_callback() {
    int ret = -1;
    if (sDrm->mIsCpp) {
        cpp::DrmManager* drm = nullptr;
        try {
            drm = new cpp::DrmManager(
                        sDrm->mConfFilePath, sDrm->mCredFilePath,
                        nullptr,
                        [&](uint32_t offset, uint32_t value) {
                            return write_drm_register(offset, value, &sDrm->pci_bar_handle);
                        },
                        [&](const string &msg) {
                            print_async_error(msg.c_str(), nullptr);
                        }
            );
            ret = DRM_ErrorCode::DRM_OK;
        } catch( const cpp::Exception& e ) {
            ret = e.getErrCode();
            std::cerr << "EXCEPTION (" << ret << "): " << e.what() << std::endl;
        }
        if (drm)
            delete drm;
    } else {
        DrmManager* pDrmManager_c = nullptr;
        ret = DrmManager_alloc( &pDrmManager_c, sDrm->mConfFilePath.c_str(), sDrm->mCredFilePath.c_str(),
                nullptr, write_drm_register, print_async_error, &sDrm->pci_bar_handle );
        if (pDrmManager_c)
            DrmManager_free( &pDrmManager_c );
    }
    return ret;
}

// Test null callback pointers
int test_null_write_callback() {
    int ret;
    if (sDrm->mIsCpp) {
        cpp::DrmManager* drm = nullptr;
        try {
            drm = new cpp::DrmManager(
                        sDrm->mConfFilePath, sDrm->mCredFilePath,
                        [&]( uint32_t offset, uint32_t* p_value ) { /* Read DRM register */
                            return read_drm_register( offset, p_value, &sDrm->pci_bar_handle );
                        },
                        nullptr,
                        [&](const string &msg) {
                            print_async_error(msg.c_str(), nullptr);
                        }
            );
            ret = DRM_ErrorCode::DRM_OK;
        } catch( const cpp::Exception& e ) {
            ret = e.getErrCode();
        }
        if (drm)
            delete drm;
    } else {
        DrmManager* pDrmManager_c = nullptr;
        ret = DrmManager_alloc( &pDrmManager_c, sDrm->mConfFilePath.c_str(), sDrm->mCredFilePath.c_str(),
                read_drm_register, nullptr, print_async_error, &sDrm->pci_bar_handle );
        if (pDrmManager_c)
            DrmManager_free( &pDrmManager_c );
    }
    return ret;
}

// Test null callback pointers
int test_null_error_callback() {
    int ret;
    if (sDrm->mIsCpp) {
        cpp::DrmManager* drm = nullptr;
        try {
            drm = new cpp::DrmManager(
                        sDrm->mConfFilePath, sDrm->mCredFilePath,
                        [&]( uint32_t offset, uint32_t* p_value ) { /* Read DRM register */
                            return read_drm_register( offset, p_value, &sDrm->pci_bar_handle );
                        },
                        [&](uint32_t offset, uint32_t value) {
                            return write_drm_register(offset, value, &sDrm->pci_bar_handle);
                        },
                        nullptr
            );
            ret = DRM_ErrorCode::DRM_OK;
        } catch( const cpp::Exception& e ) {
            ret = e.getErrCode();
        }
        if (drm)
            delete drm;
    } else {
        DrmManager* pDrmManager_c = nullptr;
        ret = DrmManager_alloc( &pDrmManager_c, sDrm->mConfFilePath.c_str(), sDrm->mCredFilePath.c_str(),
                read_drm_register, write_drm_register, nullptr, &sDrm->pci_bar_handle );
        if (pDrmManager_c)
            DrmManager_free( &pDrmManager_c );
    }
    return ret;
}

#define CHECK_VALUE(val, exp_val) if (val != exp_val) { \
    cout << __FUNCTION__ << ", " << __LINE__ << " - ERROR - bad value: got " << val << " but expect " << exp_val << endl; \
    return -1; }

#define CHECK_STRING(str, exp_str) if (str.find(exp_str) == string::npos) { \
    cout << __FUNCTION__ << ", " << __LINE__ << " - ERROR - bad message: '" << exp_str << "' not in:\n" << str << endl; \
    return -1; }


// Test different types of get  and set functions
int test_types_of_get_and_set_functions() {
    int ret = -1;
    string str;

    sDrm->create();
    try {
        sDrm->set_bool(cpp::ParameterKey::custom_field, true);
        bool b = sDrm->get_bool(cpp::ParameterKey::custom_field);
        CHECK_VALUE(b, true)

        sDrm->set_int(cpp::ParameterKey::custom_field, 0x12345678);
        int32_t i32 = sDrm->get_int(cpp::ParameterKey::custom_field);
        CHECK_VALUE(i32, 0x12345678)

        sDrm->set_uint(cpp::ParameterKey::custom_field, 0x12345678);
        uint32_t ui32 = sDrm->get_uint(cpp::ParameterKey::custom_field);
        CHECK_VALUE(ui32, 0x12345678)

        sDrm->set_int64(cpp::ParameterKey::custom_field, 0x87654321);
        int64_t i64 = sDrm->get_int64(cpp::ParameterKey::custom_field);
        CHECK_VALUE(i64, 0x87654321)

        sDrm->set_uint64(cpp::ParameterKey::custom_field, 0x87654321);
        uint64_t ui64 = sDrm->get_uint64(cpp::ParameterKey::custom_field);
        CHECK_VALUE(ui64, 0x87654321)

        float f = sDrm->get_float(cpp::ParameterKey::frequency_detection_threshold);
        sDrm->set_float(cpp::ParameterKey::frequency_detection_threshold, f);
        float fb = sDrm->get_float(cpp::ParameterKey::frequency_detection_threshold);
        CHECK_VALUE(fb, f)

        double d = (double)f;
        sDrm->set_double(cpp::ParameterKey::frequency_detection_threshold, d);
        double db = sDrm->get_double(cpp::ParameterKey::frequency_detection_threshold);
        CHECK_VALUE(db, d)

        sDrm->set_string(cpp::ParameterKey::log_message, "My test string");
        str = sDrm->get_string(cpp::ParameterKey::license_type);
        CHECK_STRING(str, "Floating/Metering")

        str = sDrm->get_string(cpp::ParameterKey::list_all);
        cout << str << endl;
        CHECK_STRING(str, "license_type")
        CHECK_STRING(str, "log_message")

        str = sDrm->get_string(cpp::ParameterKey::dump_all);
        cout << str << endl;
        CHECK_STRING(str, "license_type")
        CHECK_STRING(str, "log_message")

        string js = "{\"custom_field\":12345678}";
        sDrm->set_json_string(js);
        string jsb = "{\"custom_field\":0}";
        sDrm->get_json_string(jsb);
        CHECK_STRING(jsb, "custom_field")
        CHECK_STRING(jsb, "12345678")

        Json::Value jv;
        jv["custom_field"] = 12345678;
        jv["log_message_level"] = 5;
        sDrm->set_json_value(jv);
        Json::Value jvb;
        jvb["custom_field"] = 0;
        jvb["log_message_level"] = 0;
        sDrm->get_json_value(jvb);
        CHECK_VALUE(jvb["custom_field"].asInt(), 12345678)
        CHECK_VALUE(jvb["log_message_level"].asInt(), 5)

        ret = 0;
    } catch( const cpp::Exception& e ) {
        ret = e.getErrCode();
    }
    sDrm->destroy();
    return ret;
}

// Test out_of_range on get function
int test_get_function_out_of_range() {
    int ret = -1;
    sDrm->create();
    try {
        sDrm->get_int(cpp::ParameterKey::ParameterKeyCount+1);
    } catch( const cpp::Exception& e ) {
        ret = e.getErrCode();
    }
    sDrm->destroy();
    return ret;
}

// Test json string with bad format on get function
int test_get_json_string_with_bad_format() {
    int ret = -1;
    sDrm->create();
    try {
        string js = "{\"list_all\":0";
        sDrm->get_json_string(js);
        ret = 0;
    } catch( const cpp::Exception& e ) {
        ret = e.getErrCode();
    }
    sDrm->destroy();
    return ret;
}

// Test json string with empty string on get function
int test_get_json_string_with_empty_string() {
    int ret = -1;
    sDrm->create();
    try {
        string js = "";
        sDrm->get_json_string(js);
        ret = 0;
    } catch( const cpp::Exception& e ) {
        ret = e.getErrCode();
    }
    sDrm->destroy();
    return ret;
}

// Test out_of_range on set function
int test_set_function_out_of_range() {
    int ret = -1;
    sDrm->create();
    try {
        sDrm->set_int(cpp::ParameterKey::ParameterKeyCount+1, 1);
    } catch( const cpp::Exception& e ) {
        ret = e.getErrCode();
    }
    sDrm->destroy();
    return ret;
}

// Test json string with bad format on set function
int test_set_json_string_with_bad_format() {
    int ret = -1;
    sDrm->create();
    try {
        string js = "{\"list_all\":0";
        sDrm->set_json_string(js);
        ret = 0;
    } catch( const cpp::Exception& e ) {
        ret = e.getErrCode();
    }
    sDrm->destroy();
    return ret;
}

// Test json string with empty string on set function
int test_set_json_string_with_empty_string() {
    int ret = -1;
    sDrm->create();
    try {
        string js = "";
        sDrm->set_json_string(js);
        ret = 0;
    } catch( const cpp::Exception& e ) {
        ret = e.getErrCode();
    }
    sDrm->destroy();
    return ret;
}

// Test normal usage, especially to run valgrind for memory leak check
int test_normal_usage( string param_file) {
    int ret = -1;

    Json::Value param_json = parseJsonFile(param_file);
    uint32_t nb_running = param_json["nb_running"].asUInt();

    sDrm->create();
    try {
        sDrm->activate();
        uint32_t lic_duration = sDrm->get_uint(cpp::ParameterKey::license_duration);
        sleep(lic_duration * nb_running - 2);
        sDrm->deactivate();
        ret = 0;
    } catch( const cpp::Exception& e ) {
        ret = e.getErrCode();
    }
    sDrm->destroy();
    return ret;
}



/////////////////////////////////
/// INFRASTRUCTURE FUNCTIONS
/////////////////////////////////

void print_usage()
{
    cout << endl;
    cout << "Usage :" << endl;
    cout << "   -h         : Print this message\n" << endl;
    cout << "   -f <path>  : Specify path to configuration file" << endl;
    cout << "   -d <path>  : Specify path to credential file" << endl;
    cout << "   -s <idx>   : If server has multiple board, specify the slot index of the targeted board" << endl;
    cout << "   -t <name>  : Specify the name of test function to execute" << endl << endl;
    cout << "   -c         : When specified, the tests use the C API. Otherwise the C++ API is used" << endl;
    cout << "   -p <path>  : When specified, path to a parameter file used by the test function. File content shall be JSON-formated." << endl;
}


int main(int argc, char **argv) {

    string test_name;
    int slot_idx = 0;
    bool use_cpp = true;
    string conf_file_path, cred_file_path, param_file;

    // Shut GetOpt error messages down (return '?'):
    opterr = 0;

    // Retrieve the options:
    int opt;
    while ( (opt = getopt(argc, argv, "hs:t:f:d:cp:")) != -1 ) {  // for each option...
        switch (opt) {
            case 's': slot_idx = atoi(optarg); break;
            case 't': test_name = string(optarg); break;
            case 'f': conf_file_path = string(optarg); break;
            case 'd': cred_file_path = string(optarg); break;
            case 'c': use_cpp = false; break;
            case 'p': param_file = string(optarg); break;
            case 'h': print_usage(); return 0;
            case '?':  // unknown option...
                cerr << "Unknown option: '" << char(optopt) << "'!" << endl;
                return 1;
        }
    }
    if (conf_file_path.empty()) {
        cerr << "A valid Configuration file must be specified." << endl;
        return -1;
    }
    if (cred_file_path.empty()) {
        cerr << "A valid Credential file must be specified." << endl;
        return -1;
    }
    if (test_name.empty()) {
        cerr << "A test function name must be specified." << endl;
        return -1;
    }

    /*cout << "Using DRM Lib API version: " << DrmManager_getApiVersion() << endl;
    cout << "Using configuration file: " << conf_file_path << endl;
    cout << "Using credential file: " << cred_file_path << endl;
    if (test_name.empty())
        cout << "Running test: all" << endl;
    else
        cout << "Running test: " << test_name << endl;*/

    sDrm = new DrmManagerMaker(slot_idx, use_cpp, conf_file_path, cred_file_path);

    int ret = 0;

    try {

        if (test_name == "test_null_read_callback")
            ret = test_null_read_callback();

        if (test_name == "test_null_write_callback")
            ret = test_null_write_callback();

        if (test_name == "test_null_error_callback")
            ret = test_null_error_callback();

        if (test_name == "test_types_of_get_and_set_functions")
            ret = test_types_of_get_and_set_functions();

        if (test_name == "test_get_function_out_of_range")
            ret = test_get_function_out_of_range();

        if (test_name == "test_get_json_string_with_bad_format")
            ret = test_get_json_string_with_bad_format();

        if (test_name == "test_get_json_string_with_empty_string")
            ret = test_get_json_string_with_empty_string();

        if (test_name == "test_set_function_out_of_range")
            ret = test_set_function_out_of_range();

        if (test_name == "test_set_json_string_with_bad_format")
            ret = test_set_json_string_with_bad_format();

        if (test_name == "test_set_json_string_with_empty_string")
            ret = test_set_json_string_with_empty_string();

        if (test_name == "test_normal_usage")
            ret = test_normal_usage(param_file);

    } catch( const cpp::Exception& e) {
        cerr << "Unexpected error: " << e.what() << endl;
        ret = -1;
    }
    if (sAsyncErrorMessage.size())
        cerr << "AsyncErrorMessage=" << sAsyncErrorMessage << endl;

    delete sDrm;
    sDrm = nullptr;

    return ret;
}
