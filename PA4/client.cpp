#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <time.h>
#include <thread>
using namespace std;


void timediff (struct timeval& start, struct timeval& end){
    
}

FIFORequestChannel* create_new_channel (FIFORequestChannel* mainchan){
    char name[1024];
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    mainchan->cwrite(&m,sizeof(m));
    mainchan->cread(name,1024);
    FIFORequestChannel* newchan = new FIFORequestChannel(name,FIFORequestChannel::CLIENT_SIDE);
    return newchan;
}

void patient_thread_function(int n, int pno, BoundedBuffer* request_buffer) {
    datamsg d (pno,0.0,1);
    double resp = 0;
    for(int i = 0; i < n; i++) {
        /*chan->cwrite(&d,sizeof(datamsg));
        chan->cread(&resp,sizeof(double));
        hc->update(pno,resp);*/
        request_buffer->push((char*)&d,sizeof(datamsg));
        d.seconds+=0.004;
    }
    
    for(int i = 0; i<n; i++){
        
    }
}

void worker_thread_function(FIFORequestChannel* chan, BoundedBuffer* request_buffer, HistogramCollection* hc, int mb){
    char buf[1024];
    double resp = 0;
    
    char recvbuf[mb];
    while(true) {
        request_buffer->pop(buf,1024);
        MESSAGE_TYPE* m = (MESSAGE_TYPE*) buf;
        
        if(*m == DATA_MSG) {
            chan->cwrite(buf,sizeof(datamsg));
            chan->cread(&resp,sizeof(double));
            hc->update(((datamsg*)buf)->person,resp);
        }
        
        else if(*m == QUIT_MSG) {
            chan->cwrite(m,sizeof(MESSAGE_TYPE));
            delete chan;
            break;
        }
        
        else if(*m == FILE_MSG) {
            filemsg* fm = (filemsg*) buf;
            string fileName = (char *)(fm + 1);
            int sz = sizeof(filemsg) + fileName.size() + 1;
            chan->cwrite(buf, sz);
            chan->cread(recvbuf,mb);
            
            string recvfname = "recv/" + fileName;
            
            
            FILE* fp = fopen(recvfname.c_str(), "r+");
            fseek (fp, fm->offset, SEEK_SET);
            fwrite (recvbuf,1,fm->length,fp);
            fclose(fp);
        }
    }
}

void file_thread_function (string fname, BoundedBuffer* request_buffer,FIFORequestChannel* chan, int mb) {
    //1. create the file
    string recvfname = "recv/" + fname;
    //make the file as long as the original file
    char buf [1024];
    filemsg f (0,0);
    memcpy (buf,&f,sizeof(f));
    strcpy (buf + sizeof (f), fname.c_str());
    chan->cwrite(buf,sizeof(f) + fname.size() + 1);
    __int64_t filelength;
    chan->cread(&filelength, sizeof (filelength));
    
    FILE* fp = fopen(recvfname.c_str(),"w");
    fseek(fp,filelength,SEEK_SET);
    fclose(fp);
    //2. generate all the file messages
    filemsg* fm = (filemsg *) buf;
    __int64_t remlen = filelength;
    
    while(remlen > 0){
        fm->length = min (remlen, (__int64_t) mb);
        request_buffer->push(buf,sizeof(filemsg) + fname.size() + 1);
        fm->offset += fm->length;
        remlen -= fm->length;
    }
}



int main(int argc, char *argv[])
{
    int n = 100;    //default number of requests per "patient"
    int p = 10;     // number of patients [1,15]
    int w = 100;    //default number of worker threads
    int b = 20; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    srand(time_t(NULL));
    string fname = "10.csv";
    bool use_file = false;
    bool buff_change = false;
    int opt = -1;
    while ((opt = getopt(argc,argv,"m:n:b:w:p:f:")) != -1) {
        switch (opt) {
            case 'm':
                m = atoi(optarg);
                buff_change = true;
                break;
                
            case 'n':
                n = atoi(optarg);
                break;
                
            case 'p' : 
                p = atoi(optarg);
                break;
                
            case 'b' :
                b = atoi(optarg);
                break;
                
            case 'w' :
                w = atoi(optarg);
                break;
                
            case 'f' : 
                fname = optarg;
                use_file = true;
                break;
    }
    }
    int pid = fork();
    if (pid == 0){
		//modify this to pass along m
        //string arguments = "-m";
        //arguments = arguments + to_string(m);
        //cout << " " << arguments << " " <<  endl;
        string m2 = to_string(m);
        //execl("./server",arguments.c_str(),"-m",m,(char*)NULL); //testing
        //execl ("server", "server", (char *)NULL); //original one that was given to use
        if(buff_change) {
        execl ("server", "server","-m", m2.c_str(), (char*)NULL);
        }
        else {
            execl ("server", "server",(char*)NULL);

        }
        //execl ("server", "server", "-m", arguments, (char *)NULL); //testing
        
        cout << "stuck here" << endl;
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;
	
	
	//making the histograms and adding to the collection
	for (int i = 0; i < p; i++) {
	    Histogram* h = new Histogram(10,-2.0,2.0);
	    hc.add(h);
	}
	
	//make worker channels
	FIFORequestChannel* wchans [w];
	for (int i = 0; i < w; i++) {
	    wchans[i] = create_new_channel(chan);
	}
	
	
    struct timeval start, end;
    gettimeofday (&start, 0);
    
    /* Start all threads here */
    
	thread workers[w];
	for(int i = 0; i < w; i++){
	    workers[i] = thread(worker_thread_function, wchans[i],&request_buffer,&hc,m);
	}
    
    
    if(use_file == false) {
        
	thread patient[p];
	for(int i = 0; i < p; i++) {
	    patient[i] = thread(patient_thread_function, n, i+1, &request_buffer);
	}
	for(int i = 0; i < p; i++){
	    patient[i].join();
	}
}

	/* Join all threads here */
	if(use_file == true) {
	cout << "Reached" << endl;
	
    thread filethread (file_thread_function, fname, &request_buffer, chan, m);
	filethread.join();
	
	cout << "patient threads and file threads finished" << endl;
}

	for (int i = 0; i < w; i++) {
	    MESSAGE_TYPE q = QUIT_MSG;
	    request_buffer.push((char*) &q, sizeof(q));
	}
	
	for (int i = 0; i < w; i++) {
	    workers[i].join();
	} 

	
	
	
	/*Doing some manual cleanup to help with debugging and keep my server from runnning really slowly*/
	
	//patient thread cleanup
	/*
	for(int i =0; i < p; i++) {
	    MESSAGE_TYPE q = QUIT_MSG;
	    patient[i]->cwrite((char*)&q,sizeof(MESSAGE_TYPE));
	    //delete patient[i];
	}
	*/
	
	
	//worker thread cleanup
	/*
	for(int i = 0; i < w; i++){
	    MESSAGE_TYPE q = QUIT_MSG;
	    wchans[i]->cwrite((char*)&q,sizeof(MESSAGE_TYPE));
	    //delete wchans[i];
	}
*/
	
    gettimeofday (&end, 0);
    timediff(start,end);
    // print the results
	hc.print ();
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
}
