#include "SharedMemoryManager.hpp"

using namespace SharedMemoryManagerLibrary;

/************************************************************/
/**                  PUBLIC MEMBER FUNCTIONS               **/
/************************************************************/

const std::map<int, int> SharedMemoryManager::cShmMemKey = {
	{0, PDRM_SHARED_MEMORY_KEY_REGISTERS_PAGE},
	{1, PDRM_SHARED_MEMORY_KEY_VLNV_FILE_PAGE},
	{2, PDRM_SHARED_MEMORY_KEY_LICENSE_FILE_PAGE},
	{3, PDRM_SHARED_MEMORY_KEY_TRACE_FILE_PAGE},
	{4, PDRM_SHARED_MEMORY_KEY_METERING_FILE_PAGE},
	{5, PDRM_SHARED_MEMORY_KEY_MAILBOX_FILE_PAGE}
};


/** SharedMemoryManager
*   \brief Class constructor.
**/
SharedMemoryManager::SharedMemoryManager()
{
	// Get the shared memory segments
	for(int i=0; i<cSharedMemorySize; i++) {
		mSharedMemoryPages[i] = createSharedMemorySegment(cShmMemKey.at(i));
	}
}

/** ~SharedMemoryManager
*   \brief Class destructor.
**/
SharedMemoryManager::~SharedMemoryManager() { }


/** readSharedMemory
*   \brief Reads the register with the given name from the shared memory
*   \param[in] registerName is the name of the register to be read.
*   \param[out] registerValue is the value of the read register to be returned
**/
unsigned int SharedMemoryManager::readSharedMemory(const unsigned int registerAddress, unsigned int* registerValue) {
	// Get shared memory page index
	unsigned int pageIndex = 0;
	unsigned int dwordAddress = registerAddress >> 2;
	if (dwordAddress != PDRM_REGISTER_INDEX_PAGE)
		pageIndex = *(mSharedMemoryPages[0] + PDRM_REGISTER_INDEX_PAGE);		
	if (pageIndex >= cSharedMemorySize) {
		printf("ERROR: Invalid page index to read %d\n", pageIndex);
		return mSharedMemoryManager_UNKNOWN_PAGE_NUMBER;
	}
	if (pageIndex != 0)
		dwordAddress -= 1;
	// Read the shared memory page
	*registerValue = *(mSharedMemoryPages[pageIndex] + dwordAddress);
	printf("Read 0x%08X from @0x%04X (%d) in page %d\n", *registerValue, registerAddress, dwordAddress, pageIndex);
	return mSharedMemoryManager_NO_ERROR;
}

/** writeSharedMemory
*   \brief Writes the given value to the register with the given name in the shared memory
*   \param[in] registerName is the name of the register to write.
*   \param[in] registerValue is the value of the register to write
**/
unsigned int SharedMemoryManager::writeSharedMemory(unsigned int registerAddress, unsigned int registerValue) {
	// Get shared memory page index
	unsigned int pageIndex = 0;
	unsigned int dwordAddress = registerAddress >> 2;
	if (dwordAddress != PDRM_REGISTER_INDEX_PAGE)
		pageIndex = *(mSharedMemoryPages[0] + PDRM_REGISTER_INDEX_PAGE);
	if (pageIndex >= cSharedMemorySize) {
		printf("ERROR: Invalid page index to write %d\n", pageIndex);
		return mSharedMemoryManager_UNKNOWN_PAGE_NUMBER;
	}
	if (pageIndex != 0)
		dwordAddress -= 1;
	// Write the shared memory page
	*(mSharedMemoryPages[pageIndex] + dwordAddress) = registerValue;
	printf("Wrote 0x%08X to @0x%04X (%d) in page %d\n", registerValue, registerAddress, dwordAddress, pageIndex);
	return mSharedMemoryManager_NO_ERROR;
}


/************************************************************/
/**                  PROTECTED MEMBER FUNCTIONS            **/
/************************************************************/

/************************************************************/
/**                  PRIVATE MEMBER FUNCTIONS              **/
/************************************************************/

/** getSharedMemorySegment
*   \brief Creates and returns a pointer to the shared memory segment with the given key
*   on the currently selected page.
*   \param[in] key is the key used to connect to the shared memory segment
**/
unsigned int * SharedMemoryManager::createSharedMemorySegment(const key_t key) {
	int shmid = shmget(key, sizeof(unsigned int), 0666);
	if(shmid == -1) {
		char buffer[ 256 ];
		char * errorMsg = strerror_r( errno, buffer, 256 ); // get string message from errno
		throw std::runtime_error(std::string("Error trying to get the shared memory segment (shmget): ") + errorMsg);
	}
	// shmat to attach to shared memory
	return (unsigned int *) shmat(shmid, (void*)0, 0);
}

