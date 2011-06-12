#include "SmallFS.h"
#include <string.h>
#ifdef __linux__

#include <fcntl.h>
#include <unistd.h>
#include <endian.h>
#include <stdio.h>

#define BE32(x) be32toh(x)

#else

#include "WProgram.h"

#define BE32(x) x

#endif

#undef SMALLFSDEBUG

int SmallFS_class::begin()
{

#ifdef __linux__
	fsstart = 0;
	fd = ::open("smallfs.dat",O_RDONLY);
	if (fd<0) {
		perror("Cannot open smallfs.dat");
		return -1;
	}
#else
	struct boot_t *b = (struct boot_t*)bootloaderdata;
	fsstart = b->spiend;

#endif

	spi_enable();

	startread(fsstart);

	read(&hdr,sizeof(hdr));

	spi_disable();

#ifdef SMALLFSDEBUG

	unsigned debug;

	Serial.print("Read magic ");
	Serial.println(hdr.magic);
	Serial.print("SPI end ");
	Serial.println(fsstart);

	Serial.print("Bdata at ");
	Serial.println((unsigned)b);
	// Test read

	spi_enable();
	startread(fsstart - 8);
	read(&debug,sizeof(debug));
	Serial.print("DD1 ");
	Serial.println(debug);
	read(&debug,sizeof(debug));
	Serial.print("DD2 ");
	Serial.println(debug);
	read(&debug,sizeof(debug));
	Serial.print("DD3 ");
	Serial.println(debug);


	spi_disable();
#endif

	if(BE32(hdr.magic) == SMALLFS_MAGIC)
		return 0;
	return -1;
}


void SmallFS_class::read(void *target, unsigned size)
{
#ifdef __linux__
	if (fd>=0) {
		::read(fd,target,size);
	}
#else
	unsigned char *p=(unsigned char*)target;
	while (size--) {
		spiwrite(0);
		*p++=spiread();
	}
#endif
}

void SmallFS_class::startread_i(unsigned address)
{
#ifdef __linux__
	if (fd>=0)
		::lseek(fd,address,SEEK_SET);
#else
	spiwrite(0x0B);
    /*
	spiwrite( ((unsigned char*)address)[2] );
	spiwrite( ((unsigned char*)address)[1] );
	spiwrite( ((unsigned char*)address)[0] );
	*/
	spiwrite(address>>16);
	spiwrite(address>>8);
	spiwrite(address);
	spiwrite(0);
#endif
}


SmallFSFile SmallFS_class::open(const char *name)
{
	/* Start at root offset */
	unsigned o = fsstart + sizeof(struct smallfs_header);
	unsigned char buf[256];
	struct smallfs_entry e;

	int c;

	spi_enable();
	startread(o);

	for (c=BE32(hdr.numfiles); c; c--) {

		read(&e,sizeof(struct smallfs_entry));

		read(buf,e.fnamesize);
		buf[e.fnamesize] = '\0';
		/* Compare */
		if (strcmp((const char*)buf,name)==0) {
			spi_disable();

			return SmallFSFile(BE32(e.foffset), BE32(e.size));
		}
	}
	spi_disable();

	return SmallFSFile();
}

int SmallFSFile::read(void *buf, int s)
{
	if (!valid())
		return -1;

	if (seekpos==filesize)
		return 0; /* EOF */

	if (s + seekpos > filesize) {
		s = filesize-seekpos;
	}
	SmallFS.spi_enable();

	SmallFS.startread( seekpos + flashoffset );

	SmallFS.read(buf,s);

	SmallFS.spi_disable();
	seekpos+=s;
	return s;
}

int SmallFSFile::readCallback(int s, void (*callback)(unsigned char, void*), void *data)
{
	unsigned char c;

	if (!valid())
		return -1;

	if (seekpos==filesize)
		return 0; /* EOF */

	if (s + seekpos > filesize) {
		s = filesize-seekpos;
	}
	SmallFS.spi_enable();

	SmallFS.startread( seekpos + flashoffset );

	while (s--) {
		SmallFS.read(&c,1);
		callback(c,data);
	}

	SmallFS.spi_disable();
	seekpos+=s;
	return s;
}

void SmallFSFile::seek(int pos, int whence)
{

}

SmallFS_class SmallFS;
