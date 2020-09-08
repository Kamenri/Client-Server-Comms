#include "common.h"
#include "FIFOreqchannel.h"
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

FIFORequestChannel::FIFORequestChannel(const string _name, const Side _side, const int _bufsize) : RequestChannel(_name, _side, _bufsize)
{
	rfname = "fifo_" + name + "1";
	wfname = "fifo_" + name + "2";
		
	if (_side == SERVER_SIDE) {
		wfd = open_pipe(rfname, O_WRONLY);
		rfd = open_pipe(wfname, O_RDONLY);
	}
	else {
		rfd = open_pipe(rfname, O_RDONLY);
		wfd = open_pipe(wfname, O_WRONLY);
	}
}

FIFORequestChannel::~FIFORequestChannel()
{ 
	close(wfd);
	close(rfd);

	remove(rfname.c_str());
	remove(wfname.c_str());
}

int FIFORequestChannel::open_pipe(string _pipe_name, int mode)
{
	mkfifo (_pipe_name.c_str (), 0600);
	int fd = open(_pipe_name.c_str(), mode);
	if (fd < 0){
		EXITONERROR(_pipe_name);
	}
	return fd;
}

int FIFORequestChannel::cread(void* ptr, int len)
{
	return read(rfd, ptr, len); 
}

int FIFORequestChannel::cwrite(void* ptr, int len)
{
	return write(wfd, ptr, len);
}

