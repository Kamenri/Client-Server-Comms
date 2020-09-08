#ifndef _SHMBB_H_
#define _SHMBB_H_

#include <semaphore.h>
#include <fcntl.h>
#include "common.h"


class SHMBB
{
private:
    string name;
    sem_t* producerdone;
    sem_t* consumerdone;

    int shmfd;
    char *data;
	int bufsize;

public:
	SHMBB(const string _name, const int _bufsize) 
    {
        name = _name;
        bufsize = _bufsize;

        int shmfd = shm_open(name.c_str(), O_RDWR | O_CREAT, 0644);
        if (shmfd < 0) {
            EXITONERROR("SHM Open Failed");
        }

        if (ftruncate(shmfd, bufsize) == -1)     // set the length
            EXITONERROR("SHM Truncate Failed");

        data = (char *) mmap(NULL, bufsize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
        if (data == MAP_FAILED)
            EXITONERROR("SHM Map Failed");
        
        producerdone = sem_open((name + "1").c_str(), O_CREAT, 0644, 0);
        consumerdone = sem_open((name + "2").c_str(), O_CREAT, 0644, 1);
    } 

	~SHMBB()
    {
        munmap(data, bufsize);
        close(shmfd);

        shm_unlink(name.c_str());

        sem_close(producerdone);
        sem_close(consumerdone);

        sem_unlink((name + "1").c_str());
        sem_unlink((name + "2").c_str());
    }

	int push(char *msg, int len)
    {
        sem_wait(consumerdone);
        memcpy(data, msg, len);
        sem_post(producerdone);

        return len;
    }

	int pop(char *msg , int len)
    {
        sem_wait(producerdone);
        memcpy(msg, data, len);
        sem_post(consumerdone);

        return len;
    }
};

#endif
