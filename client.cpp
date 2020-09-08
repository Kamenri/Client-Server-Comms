#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "RequestChannel.h"
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h"

using namespace std;

HistogramCollection hc;


RequestChannel* create_new_channel(RequestChannel* main_channel, string ival, int bufferCapacity)
{
	MESSAGE_TYPE m = NEWCHANNEL_MSG;
	main_channel->cwrite (&m, sizeof(MESSAGE_TYPE));

    char buf[1024];
	int nbytes = main_channel->cread (buf, 1024);
	buf[nbytes] = '\0';

    RequestChannel* newchan = NULL;
    
    if (ival == "f")
        newchan = new FIFORequestChannel(buf, RequestChannel::CLIENT_SIDE, bufferCapacity);
    else if (ival == "q")
        newchan = new MQRequestChannel(buf, RequestChannel::CLIENT_SIDE, bufferCapacity);
    else if (ival == "s")
        newchan = new SHMRequestChannel(buf, RequestChannel::CLIENT_SIDE, bufferCapacity);
    
    return newchan;
}

void* patient_thread_function(BoundedBuffer *request_buffer, int patient_no, int number_of_request)
{
    datamsg d (patient_no, 0.0, 1);

    for (int k = 0; k < number_of_request; ++k)
    {
        request_buffer->push((char *) &d, sizeof(datamsg));
        d.seconds += 0.004;
    }
}

void* file_thread_function(string filename, BoundedBuffer* request_buffer, RequestChannel* chan, int bufferCapacity)
{
    char buf[1024];

    // 1. Find the size of the file from the server
    filemsg f (0, 0);
    memcpy(buf, &f, sizeof(filemsg));
    strcpy(buf + sizeof(filemsg), filename.c_str());

    int sz = sizeof(filemsg) + filename.size() + 1;
    chan->cwrite(buf, sz);

    __int64_t filesize;
    chan->cread(&filesize, sizeof(__int64_t));

    // 2. Create the Empty file with its original size
    string recvFilename = "recv/" + filename;
    FILE *fp = fopen(recvFilename.c_str(), "w");
    fseek(fp, filesize, SEEK_SET);
    fclose(fp);

	// 3. Loop to generate the file messages
    __int64_t remLen = filesize;

    f.offset = 0;
    while (remLen > 0)
    {
        f.length = min(remLen, (__int64_t) bufferCapacity);

        memcpy(buf, &f, sizeof(filemsg));
        strcpy(buf + sizeof(filemsg), filename.c_str());
        request_buffer->push(buf, sz);

        f.offset += f.length;
        remLen -= f.length;
    }
}

void* worker_thread_function(RequestChannel* chan, BoundedBuffer* request_buffer, HistogramCollection* hc, int bufferCapacity)
{
    char buf[1024];
    double response;

    char revbuf[bufferCapacity];

    while (true)
    {
        request_buffer->pop(buf, 1024);
        MESSAGE_TYPE* m = (MESSAGE_TYPE *) buf;

        if (*m == DATA_MSG)
        {
            chan->cwrite(buf, sizeof(datamsg));
            chan->cread(&response, sizeof(double));

            datamsg *d = (datamsg*) buf;
            hc->update(d->person, response);
        }
        else if (*m == FILE_MSG)
        {
            filemsg *f = (filemsg*) buf;
            string filename = (char *) (f + 1);
            int sz = sizeof(filemsg) + filename.size() + 1;

            chan->cwrite(buf, sz);
            chan->cread(&revbuf, bufferCapacity);

            string recvFilename = "recv/" + filename;
            FILE *fp = fopen(recvFilename.c_str(), "r+");
            fseek(fp, f->offset , SEEK_SET);
            fwrite(revbuf, 1, f->length, fp);
            fclose(fp);
        }
        else if (*m == QUIT_MSG)
        {
            chan->cwrite (m, sizeof(MESSAGE_TYPE));
            delete chan;
            break;
        }
    }
}

void histogram_update(int signal)
{
    system("clear");    // Clear the screen
    hc.print();

    alarm(2);   // Generate the SIGALRM after 2 seconds
}


int main(int argc, char *argv[])
{
    int n = 100;    //default number of requests per "patient"
    int p = 10;     // number of patients [1,15]
    int w = 100;    //default number of worker threads
    int b = 20; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    string filename = "";

    srand(time_t(NULL));

    int opt = -1;    
    string ival = "f";

	while ((opt = getopt(argc, argv, "m:n:b:w:p:f:i:")) != -1) 
	{
		switch (opt) 
		{
			case 'm':
				m = atoi(optarg);
				break;
				
			case 'n':
				n = atoi(optarg);
				break;

			case 'p':
				p = atoi(optarg);
				break;

			case 'b':
				b = atoi(optarg);
				break;

			case 'w':
				w = atoi(optarg);
				break;

            case 'f':
			    filename = optarg;
                break;

            case 'i':
                ival = optarg;
                break;
		}
	}

    int pid = fork();
    if (pid == 0)
    {
		// pass the BufferCapacity to server
        string buf = "-m" + to_string(m);
        string buf2 = "-i" + ival;
        execl ("server", "server", buf.c_str(), buf2.c_str(), NULL);
    }

    BoundedBuffer request_buffer(b);

    RequestChannel *chan = NULL;

    // create Main Channel to initialize the communication
    if (ival == "f")
        chan = new FIFORequestChannel("control", RequestChannel::CLIENT_SIDE, m);
    else if (ival == "q")
        chan = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE, m);
    else if (ival == "s")
        chan = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE, m);

    if (filename.length() > 0)
    {
        // Only one thread needed for File Message
        p = 1;
    }
    else
    {
        // create Histograms and adding to the Histogram Collection
        for (int i = 0; i < p; ++i)
        {
            Histogram *h = new Histogram(10, -2.0, 2.0);
            hc.add(h);
        }

        signal(SIGALRM, histogram_update);      // Signal Handler for SIGALRM
        alarm(2);   // Generate the SIGALRM after 2 seconds
    }

    // create Worker Channels
    RequestChannel* wchans[w];
    for (int i = 0; i < w; ++i) {
        wchans[i] = create_new_channel(chan, ival, m);
    }
    
    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */
    thread patients[p];

    // create Patient threads that will put the data into BoundedBuffer
    if (filename.length() > 0)
    {
        patients[0] = thread(file_thread_function, filename, &request_buffer, chan, m);
    }
    else
    {
        for (int i = 0; i < p; ++i) {
            patients[i] = thread(patient_thread_function, &request_buffer, (i + 1), n);
        }
    }
    
    // create Worker threads that will send the data thru the RequestChannel
    thread workers[w];
    for (int i = 0; i < w; ++i) {
        workers[i] = thread(worker_thread_function, wchans[i], &request_buffer, &hc, m);
    }

    for (int i = 0; i < p; ++i) {
        patients[i].join();
    }

    // push QUIT_MSG into request_buffer so Worker threads will terminate
    for (int i = 0; i < w; ++i) 
    {
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char *) &q, sizeof(MESSAGE_TYPE));
    }

    for (int i = 0; i < w; ++i) {
        workers[i].join();
    }

    /* Join all threads here */
    gettimeofday (&end, 0);

    // print the results
    if (filename.length() == 0)
    {
        system("clear");
        hc.print();
        cout << endl;
    }

    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite((char *) &q, sizeof(MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;

}
