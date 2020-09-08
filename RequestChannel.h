#ifndef _RequestChannel_H_
#define _RequestChannel_H_

#include "common.h"


class RequestChannel
{
public:
	enum Side { SERVER_SIDE, CLIENT_SIDE };

protected:
	string name;
	Side side;
	int bufsize;
	
public:
	RequestChannel(const string _name, const Side _side, const int _bufsize)
    {
        name = _name;
        side = _side;
		bufsize = _bufsize;
    }

	virtual ~RequestChannel() { }

	virtual int cread(void *ptr, int len) = 0;
	virtual int cwrite(void *ptr , int len) = 0;
};

#endif