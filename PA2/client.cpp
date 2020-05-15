/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <iostream>
#include <string>
#include <cstring>
#include <stdio.h>
#include <sys/wait.h>

using namespace std;

void retrieve_Single_Data_Point(FIFORequestChannel& c, datamsg dat){
    datamsg retrieve = dat;
    c.cwrite(&retrieve, sizeof(datamsg));
    char* fetched = new char[MAX_MESSAGE];
    int nbytes = c.cread((char*)fetched,MAX_MESSAGE);
    cout << *((double*)fetched) << endl;
    
}

int main(int argc, char *argv[]) {
    int n = 100;    // default number of requests per "patient"
	int p = 15;	// number of patients
    srand(time_t(NULL));
    
    
    // sending a non-sense message, you need to change this
    string file_name;
    struct timeval start, end;
    double stamp;
    int eno;
    int option;
    int max_msg = 256;

    while((option = getopt(argc, argv,"p:t:e:f:m:")) != -1)
        switch(option) {
            
            case 'p' :
                //fill this with code for P
                //this statement is for the patient that we want. Our patients range from [1 - 15]
                p = (int)atoi(optarg);
                //cout << "This is the person that is being requested " << p << endl;
                break;
                
            case 't' :
                //fill this with code for T
                //this statement is for the time stamp we are requesting at this has a range of [0.00 - 59.996]
                stamp = atof(optarg);
                //cout << "This is the range of time stamps " << stamp << endl;
                break;
                
            case 'e' :
                //this statement is for the ECG number.
                //there are only two contact points so this must be from [1 - 2]
                //fill this with code for E
                eno = (double)atoi(optarg);
                //cout << "This is the contact point requested for ECG "<< eno << endl;
                break;
                
            case 'f' :
                //this statement is for the file request input
                file_name = string(optarg);
            
            case 'm' : 
                //this is to change the the size of our buffercapacity
                max_msg = (int)atoi(optarg);
                
        }
        
        if(fork()==0) {
            cout << "running child process " << endl;
            string arguments = "-m:";
            arguments = arguments + to_string(max_msg);
            cout << arguments << endl;
            execl("./server",arguments.c_str(),(char*)NULL);
        }
        
        
        else {
        cout << "parent process is currently running" << endl;
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
    

    datamsg msgSend = datamsg(9,0.008,1);
    
    chan.cwrite(&msgSend, sizeof(msgSend));
    double data;
    chan.cread((char*)&data,sizeof(double));
    
    // cout << "Response " << data << endl;
    
    ofstream myfile;
    
    gettimeofday(&start, NULL);
    
    myfile.open("received/x1.csv");
    double i = 0;
    
    cout << "Fetching data points...." << endl;
    
    while (i <= 59.996) {
        
        myfile << i << ",";
        
        datamsg ecg1 = datamsg(p,i,1);
        datamsg ecg2 = datamsg(p,i,2);
        
        
        chan.cwrite(&ecg1, sizeof(datamsg));
        double data1;
        chan.cread((char*)&data1,sizeof(double));
        
        myfile << data1 << ",";
        
        chan.cwrite(&ecg2, sizeof(datamsg));
        double data2;
        chan.cread((char*)&data2, sizeof(double));
        
        myfile << data2 << "\n";
        
        i = i + .004;
    }
    
    myfile.close();
    cout << "All data points have been fetched" << endl;
    
    gettimeofday(&end,NULL);
    
    double readTime;
    
    readTime = (end.tv_sec - start.tv_sec) * 1e6;
    readTime = (readTime + (end.tv_usec - start.tv_usec)) * 1e-6;
    
    cout << "Time taken to fetch all data points for 1.csv : " << fixed << readTime << setprecision(6);
    cout << " seconds" << endl;

    
    if(file_name != ""){
        gettimeofday(&start,NULL);
        
        
        int buffCap = (file_name.length() + sizeof(filemsg) + 1);
        char* buff = new char[buffCap];
        filemsg* message_Req = (filemsg*)buff;
        message_Req->offset = 0;
        message_Req->length = 0;
        message_Req->mtype = FILE_MSG;
        
        strcpy(buff + sizeof(filemsg),file_name.c_str());
        chan.cwrite(message_Req,buffCap);
        __int64_t file_size;
        chan.cread((char*)&file_size,max_msg);
        __int64_t request_num = ceil((double)file_size/(double)max_msg);
        
        cout << "file size: " << (__int64_t)file_size << endl;
        cout << "number of requests: " << request_num << endl;
        
        ofstream store_folder;
        
        store_folder.open(("~/environment/PA2/received/"+file_name), ios::out | ios::binary);
        
        for(__int64_t i = 0; i < request_num; i++){
            int sizeofMsg = max_msg;
            __int64_t offset = i * sizeofMsg;
            
            if((sizeofMsg + offset) > file_size) {
                sizeofMsg = file_size - offset;
            }
            
            message_Req->offset = offset;
            message_Req->length = sizeofMsg;
            
            chan.cwrite(buff,buffCap);
            char* response = new char(sizeofMsg);
            chan.cread(response,sizeofMsg);
            store_folder.write(response,sizeofMsg);
        }
        
        store_folder.close();
        gettimeofday(&end, NULL);
        double file_fetch_time;
        
        file_fetch_time = (end.tv_sec + end.tv_usec * 1e-6) - (start.tv_sec + start.tv_usec * 1e-6);
            cout << "Time taken to fetch file: " << file_fetch_time << " seconds."  << endl;
    }
    
    
    MESSAGE_TYPE *c = new MESSAGE_TYPE(NEWCHANNEL_MSG);
    chan.cwrite(c, sizeof(MESSAGE_TYPE));
    
    char* new_channel_buff = new char[sizeof(FIFORequestChannel)]; 
    chan.cread((char*)new_channel_buff,sizeof(FIFORequestChannel));
    string new_channel_name = (string)new_channel_buff;
    
    cout << "New channel name: "<< new_channel_name << endl;
    FIFORequestChannel newchan(new_channel_name,FIFORequestChannel::CLIENT_SIDE);
    
    //find just a single data point with the data given
    
    datamsg single_point = datamsg(2,0,1);
    retrieve_Single_Data_Point(newchan,single_point);
    single_point = datamsg(2,0,2);
    retrieve_Single_Data_Point(newchan,single_point);
    
    

    
    /*
    string file_name;
    char* buf_F = new char[file_name.size() + 1 + sizeof(filemsg)];
    filemsg* f = (filemsg*) buf_F;
    f->mtype = FILE_MSG;
    f->offset = 0;
    f->length = 5000;
    
    strcpy(buf_F + sizeof(filemsg), file_name.c_str());
    chan.cwrite(buf_F,sizeof(filemsg) + file_name.size() + 1);
    
    cout << "Size of my buffer : " << sizeof(buf_F) << endl; 
    cout << "Size of raw file : " << f->length << endl;  
    */
    
    
    
    char buf [MAX_MESSAGE];
    char x = 55;
    chan.cwrite (&x, sizeof (x));
    int nbytes = chan.cread (buf, MAX_MESSAGE);
    // closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));
    wait(NULL);
        }
        
}
