#include "common.h"
#include "SHMreqchannel.h"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

SHMRequestChannel::SHMRequestChannel(const string _name, const Side _side, int _bufsize) : RequestChannel(_name, _side, _bufsize)
{
	s1 = "/bb_" + name + "1";
	s2 = "/bb_" + name + "2";
		
	if (_side == SERVER_SIDE) {
		b1 = new SHMBB(s1, bufsize);
		b2 = new SHMBB(s2, bufsize);
	}
	else {
		b1 = new SHMBB(s2, bufsize);
		b2 = new SHMBB(s1, bufsize);
	}
}

SHMRequestChannel::~SHMRequestChannel()
{ 
	delete b1;
	delete b2;
}

int SHMRequestChannel::cread(void* ptr, int len)
{
	return b1->pop((char *) ptr, len);
}

int SHMRequestChannel::cwrite(void* ptr, int len)
{
	return b2->push((char *) ptr, len);
}

