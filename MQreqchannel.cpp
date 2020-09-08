#include "common.h"
#include "MQreqchannel.h"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

MQRequestChannel::MQRequestChannel(const string _name, const Side _side, int _bufsize) : RequestChannel(_name, _side, _bufsize)
{
	rfname = "/mq_" + name + "1";
	wfname = "/mq_" + name + "2";
		
	if (_side == SERVER_SIDE) {
		wfd = open_pipe(rfname, O_WRONLY);
		rfd = open_pipe(wfname, O_RDONLY);
	}
	else {
		rfd = open_pipe(rfname, O_RDONLY);
		wfd = open_pipe(wfname, O_WRONLY);
	}
}

MQRequestChannel::~MQRequestChannel()
{ 
	mq_close(wfd);
	mq_close(rfd);

	mq_unlink(wfname.c_str());
	mq_unlink(rfname.c_str());
}

int MQRequestChannel::open_pipe(string _pipe_name, int mode)
{
	struct mq_attr attr { 0, 5, bufsize, 0 };

	int fd = mq_open(_pipe_name.c_str(), O_RDWR | O_CREAT, 0664, &attr);
	if (fd < 0) {
		EXITONERROR("Message Queue Create Failed\n");
	}
	return fd;
}

int MQRequestChannel::cread(void* ptr, int len)
{
	int nb = mq_receive(rfd, (char *) ptr, bufsize, 0); 
	if (nb < 0) {
		EXITONERROR("MQ Receive Failed\n");
	}

	return nb; 
}

int MQRequestChannel::cwrite(void* ptr, int len)
{
	int nb = mq_send(wfd, (char *) ptr, len, 0);
	if (nb < 0) {
		EXITONERROR("MQ Send Failed\n");
	}

	return nb; 
}

