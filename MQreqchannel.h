
#ifndef _MQreqchannel_H_
#define _MQreqchannel_H_

#include <mqueue.h>
#include "common.h"
#include "RequestChannel.h"

class MQRequestChannel : public RequestChannel
{
private:
	int wfd;
	int rfd;
	
	string rfname;
	string wfname;

public:
	MQRequestChannel(const string _name, const Side _side, int _bufsize);
	~MQRequestChannel();

	int cread(void *ptr, int len);
	int cwrite(void *ptr , int len);

	int open_pipe(string _pipe_name, int mode);
};

#endif
