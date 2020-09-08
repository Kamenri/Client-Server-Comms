
#ifndef _FIFOreqchannel_H_
#define _FIFOreqchannel_H_

#include "common.h"
#include "RequestChannel.h"

class FIFORequestChannel : public RequestChannel
{
private:
	int wfd;
	int rfd;
	
	string rfname;
	string wfname;

public:
	FIFORequestChannel(const string _name, const Side _side, const int _bufsize);
	~FIFORequestChannel();

	int cread(void *ptr, int len);
	int cwrite(void *ptr , int len);

	int open_pipe(string _pipe_name, int mode);
};

#endif
