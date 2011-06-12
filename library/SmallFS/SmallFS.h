#ifndef __SMALLFS_H__
#define __SMALLFS_H__

#ifdef __linux__

#else
#include "zpuino.h"
#endif
/** SmallFS filesystem magic */
#define SMALLFS_MAGIC 0x50411F50


struct smallfs_header {
	unsigned int magic /** big-endian, magic number **/;
	unsigned int numfiles;
}__attribute__((packed));

struct smallfs_entry {
	unsigned int offset;
	unsigned int size;
	unsigned char namesize;
	char name[0];
} __attribute__((packed));

extern "C" void *bootloaderdata;
struct boot_t {
	unsigned int spiend;
};
/**
 * @brief SmallFS File Class
 */
class SmallFSFile
{
public:
	SmallFSFile(): flashoffset(-1) {}
	SmallFSFile(unsigned o,unsigned size): flashoffset(o),filesize(size),seekpos(0) {}
	/**
	 * @brief Check if file was successfuly opened.
	 * @return true on success, false otherwise
     */
	bool valid() { return flashoffset!=-1; }

	/**
	 * @brief Read a chunk of data from file.
	 * @param buf The buffer where to store data
	 * @param size The number of bytes to read from file.
	 * @return The number of bytes read, 0 for EOF.
	 */
	int read(void *buf, int size);
	/**
	 * @brief Seek current file position
	 * @param pos The required position
	 * @param whence Where to perform seek. Either SEEK_SET, SEEK_CUR or SEEK_END
	 */
	void seek(int pos, int whence);
	/**
	 * @brief Get the file size.
	 * @return The file size.
     */
	inline int size() const { return filesize; }
	/**
	 * @brief Read a chunk of data from file, using a callback function.
	 * The function will be called for every byte read.
	 * @param size The number of bytes to read from file.
	 * @param callback The callback function to call
	 * @param data The data parameter to pass to callback function.
	 * @return The number of bytes read, 0 for EOF.
	 */
	
	int readCallback(int size, void (*callback)(unsigned char, void*), void *data);

private:
	int flashoffset;
	int filesize;
    int seekpos;
};

/**
 * @brief Main filesystem class
 */
class SmallFS_class {
    friend class SmallFSFile;
public:
	/**
	 * @brief Initialize the SmallFS filesystem
	 * @return 0 on success, -1 otherwise
	 */
	int begin();

protected:
	void spi_disable()
	{
#ifndef __linux__
		digitalWrite(SPI_FLASH_SEL_PIN,HIGH);
#endif
	}

	static void spi_enable()
	{
#ifndef __linux__
		digitalWrite(SPI_FLASH_SEL_PIN,LOW);
#endif
	}

	static inline void spiwrite(unsigned int i)
	{
#ifndef __linux__
		SPIDATA=i;
#endif
	}

	static inline unsigned int spiread()
	{
#ifndef __linux__
		return SPIDATA;
#else
		return 0;
#endif
	}

	void startread(unsigned address)
	{
/*		if (__builtin_constant_p(address)) {
			spiwrite(0x0B);
			spiwrite(address >> 16);
			spiwrite(address >> 8);
			spiwrite(address);
			spiwrite(0);
		} else {*/
		startread_i(address);
        /*
		 } */
	}
protected:
	void read(void *target, unsigned size);
	void startread_i(unsigned address);

public:
	/**
	 * @brief Open a file on the filesystem.
	 * @param name The file name
	 * @return A new SmallFSFile object. You should call valid() to check
	 * if file was successfully open.
     */
	SmallFSFile open(const char *name);

private:
	struct smallfs_header hdr;
	unsigned fsstart;
#ifdef __linux__
	int fd;
#endif
};

extern SmallFS_class SmallFS;

#endif
