#ifndef _SHMreqchannel_H_
#define _SHMreqchannel_H_

#include <sys/mman.h>
#include "common.h"
#include "RequestChannel.h"
#include "SHMBB.h"

class SHMRequestChannel : public RequestChannel
{
private:
	SHMBB *b1;
	SHMBB *b2;

	string s1;
	string s2;

public:
	SHMRequestChannel(const string _name, const Side _side, int _bufsize);
	~SHMRequestChannel();

	int cread(void *ptr, int len);
	int cwrite(void *ptr , int len);
};

#endif
