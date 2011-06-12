#include "smallfs.h"
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <endian.h>

struct fileinfo {
	std::string name;
	int size;
	fileinfo(const std::string &n, int s): name(n), size(s) {}
	fileinfo() {}
};

int help()
{
	fprintf(stderr,"Usage: mksmallfs outputfile directory\n");
	return -1;
}

int main(int argc,char **argv)
{
	struct dirent *e;
	const char *targetfilename;
	struct smallfs_header hdr;
    struct stat st;
	unsigned int rootsize = sizeof(hdr);
	unsigned int currentoffset = 0;


	std::vector<fileinfo> filestopack;
	std::vector<fileinfo>::iterator i;

	std::string fname;

	if (argc<3)
		return help();

	targetfilename=argv[1];

	DIR *dh = opendir(argv[2]);
	if (NULL==dh) {
		perror("opendir");
		return help();
	}

	while ((e=readdir(dh))) {
		if (e->d_type==DT_DIR && e->d_name[0] == '.') {
			continue;
		}
		if (e->d_type!=DT_REG) {
			fprintf(stderr,"Unsupported file '%s'\n", e->d_name);
			return -1;
		}
		fname = argv[2];
		fname += "/";
		fname += e->d_name;
		if (stat(fname.c_str(),&st)<0) {
			fprintf(stderr,"%s: ", fname.c_str());
			perror("stat");
			return -1;
		}
		rootsize += strlen(e->d_name);

		filestopack.push_back( fileinfo(e->d_name, st.st_size) );
	}
	closedir(dh);

    rootsize += sizeof(smallfs_entry) * filestopack.size();

	int fd = open(targetfilename,O_TRUNC|O_CREAT|O_WRONLY,0666);
	if (fd<0) {
		fprintf(stderr,"%s: ", targetfilename);
		perror("open");
		return -1;
	}

	hdr.magic = htobe32(SMALLFS_MAGIC);
	hdr.numfiles = htobe32(filestopack.size());

	if (write(fd,&hdr,sizeof(hdr))!=sizeof(hdr)) {
		perror("write");
		return -1;
	}

	/* Write directory entries */

	for (i=filestopack.begin();i!=filestopack.end();i++) {
		struct smallfs_entry e;
		e.fnamesize = i->name.length();
		e.foffset = htobe32(rootsize);
		e.size = htobe32(i->size);

		rootsize += i->size;

		if (write(fd,&e,sizeof(e))!=sizeof(e)) {
			perror("write");
			return -1;
		}
		if (write(fd,i->name.c_str(), i->name.length())!=i->name.length()) {
			perror("write");
			return -1;
		}
	}

    /* Write files */

	for (i=filestopack.begin();i!=filestopack.end();i++) {
		unsigned char buf[8192];
		int infd;
		int n;
		std::string rfname = argv[2] ;
		rfname += "/";
		rfname += i->name;

		infd = open(rfname.c_str(), O_RDONLY);
		if (infd<0) {
			perror("open");
			return -1;
		}
		do {
			n=read(infd, buf, sizeof(buf));
			if (n<=0)
				break;
			if (write(fd,buf,n)<0) {
				perror("write");
				return -1;
			}
		} while (1);
		close(infd);

	}

	close(fd);
	printf("Packed %d files sucessfully\n",filestopack.size());
}
