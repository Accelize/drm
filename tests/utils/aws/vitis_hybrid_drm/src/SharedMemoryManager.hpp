#ifndef __SHARED_MEMORY_MANAGER_HPP__
#define __SHARED_MEMORY_MANAGER_HPP__

#include <sys/shm.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <map>

#define PDRM_REGISTER_INDEX_PAGE     0

#define PDRM_SHARED_MEMORY_KEY_FILE_PATH             "/usr/lib/drm-controller/shared-memory-file"
#define PDRM_SHARED_MEMORY_KEY_ID_REGISTERS_PAGE     1
#define PDRM_SHARED_MEMORY_KEY_ID_VLNV_FILE_PAGE     2
#define PDRM_SHARED_MEMORY_KEY_ID_LICENSE_FILE_PAGE  3
#define PDRM_SHARED_MEMORY_KEY_ID_TRACE_FILE_PAGE    4
#define PDRM_SHARED_MEMORY_KEY_ID_METERING_FILE_PAGE 5
#define PDRM_SHARED_MEMORY_KEY_ID_MAILBOX_FILE_PAGE  6

/**
*   \enum tSharedMemoryManagerErrorCode
*   \brief Enumeration for error code values.
**/
typedef enum tSharedMemoryManagerErrorCode {
  mSharedMemoryManager_NO_ERROR = 0,                 /**<No error.**/
  mSharedMemoryManager_UNKNOWN_PAGE_NUMBER           /**<The page number found in the page register is unknown.**/
} tSharedMemoryManagerErrorCode;

/**
*   \namespace SharedMemoryManagerLibrary
**/
namespace SharedMemoryManagerLibrary {

  /**
  *   \class    SharedMemoryManager SharedMemoryManager.hpp "shared-memoryManager/DrmControllerClient.hpp"
  *   \brief    Class SharedMemoryManager is used by the client application on the host to link the SDK with
  *                                       the shared memory of the pDRMController
  **/
  class SharedMemoryManager {

    public:

      /** SharedMemoryManager
      *   \brief Class constructor.
      **/
      SharedMemoryManager();

      /** ~SharedMemoryManager
      *   \brief Class destructor.
      **/
      ~SharedMemoryManager();

      /** readSharedMemory
      *   \brief Reads the register with the given name from the shared memory
      *   \param[in] registerName is the name of the register to be read.
      *   \param[out] registerValue is the value of the read register to be returned
      **/
      unsigned int readSharedMemory(unsigned int registerAddress, unsigned int *registerValue);

      /** writeSharedMemory
      *   \brief Writes the given value to the register with the given name in the shared memory
      *   \param[in] registerName is the name of the register to write.
      *   \param[in] registerValue is the value of the register to write
      **/
      unsigned int writeSharedMemory(unsigned int registerAddress, unsigned int registerValue);

    protected:

    private:

      static const int cSharedMemorySize = 6;

      static const std::map<int, int> cShmMemKey;

      volatile unsigned int *mSharedMemoryPages[cSharedMemorySize];

      /** getSharedMemorySegment
      *   \brief Creates and returns a pointer to the shared memory segment with the given key
      *   on the currently selected page.
      *   \param[in] key is the ID of the key used to connect to the shared memory segment
      **/
      unsigned int * createSharedMemorySegment(const int keyId);

  }; // class SharedMemoryManager

} // namespace SharedMemoryManagerLibrary

#endif // __SHARED_MEMORY_MANAGER_HPP__
