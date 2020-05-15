#include "unistd.h"
#include <string>
#include <limits.h>
#include <pwd.h>
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <pwd.h>

#define MAXCOM 1000 //max number of letters that can be input
#define MAXLIST 100 //max number of commands that can be input

#define clear() printf("\033[H\033[J")

using namespace std;

vector<string> pipeRedirection(string str, string delis) {
	vector<string> elems;

	string delimiters = delis; 
	size_t pos = 0; 
	
	stringstream stringStream(str);
	string line;
	while(getline(stringStream, line)) 
	{
	    size_t prev = 0, pos;
	    while ((pos = line.find_first_of(delimiters, prev)) != string::npos)
	    {
	        if (pos > prev)
	            elems.push_back(line.substr(prev, pos-prev));
	        prev = pos+1;
	    }
	    if (prev < line.length())
	        elems.push_back(line.substr(prev, string::npos));
	}
	
	//Gets rid of whitespaces in each string
	for(int i = 0; i<elems.size(); i++){
		while(1){
			if(elems.at(i).at(0) == ' '){
				elems.at(i).erase(0, 1); 
			} else { 
				break; 
			}
		}
		while(1){
			if(elems.at(i).at(elems.at(i).size()-1) == ' '){
				elems.at(i).erase(elems.at(i).size()-1, elems.at(i).size()); 
			}else{
				break;
			}
		}
	}

	return elems; 
}


/*
void userPrompt() {
    clear();
    printf("\n\n--------------------------------------------------------------");
    printf("\n\n\t          [ Welcome to my shell! ]");
    printf("\n\n--------------------------------------------------------------");
    
    char* username = getenv("USER");
    printf("\n\n\n\n current user: @%s", username);
    printf("\n");
    sleep(1);
    clear();
}

vector<string> remove_whitespace(vector<string> parsed_input) {
    for(int i = 0; i < parsed_input.size(); i++){
        if(parsed_input[i] == "")
        parsed_input.erase(parsed_input.begin() + i);
    }
    return parsed_input;
}
*/


bool findAmb(vector<string> vec){
	for(string s : vec){
		if(s.find('&') != string::npos){
			return true; 
		}
	}
	return false; 
}

bool findStringChar(string s, char x){
	return (s.find(x) != string::npos); 	
}

bool findSubStr(string str, string s){
	return (str.find(s) != string::npos); 
}

string shellExe(){
	
	string userInput = "";  
	//vector<vector<string> > commands; 

	vector<string> commandVec; 

	cout<<endl;
	//Start Shell: 
	bool promptUser = true; 

	char* args[50];
	char bBuffer[BUFSIZ], *argPtr = NULL, *stringPtr; 
	ssize_t rBytes; 
	int argCount; 

	const int PIPE_READ = 0; 
	const int PIPE_WRITE = 1; 

	int fds [2];

	do {
		//Prompt the user for input
		if(promptUser){
		    char cwd[PATH_MAX];
			cout << getpwuid(geteuid()) -> pw_name << ":~" << getcwd(cwd,sizeof(cwd)) << " $ "; //this string allows me to print the entire working directory like a normal terminal line, this will keep track of your location essentially.
		}
		//Get the user commands and translate them into iputs.
		getline(cin, userInput);
		
		
		//make each command universal as a char*[]
		//Now we can properly work with user inputs and recognize commands and different symbols.
		commandVec = pipeRedirection(userInput, "|"); 
		bool isAmb = false; 

		if(fork() == 0){
			int termSize = commandVec.size(); 
			isAmb = findAmb(commandVec); 
			if(isAmb) termSize = commandVec.size() -1; 
			for(int i = 0; i<termSize -1; i++) {
				
				if(findStringChar(commandVec.at(i), '>')) {
					vector<string> tempV; 
					tempV = pipeRedirection(commandVec.at(i), ">");	
					//here we are casting the symbol to have double quotes and it will be easy to handle intputs this way, since we are making them universal.
					string str1, str2; 
					str1 = tempV.at(0); 
					str2 = tempV.at(1); 

					int ff = open(str2.c_str(), O_CREAT|O_WRONLY, S_IRUSR | S_IWUSR); 
					dup2(ff, 1); 
					
					//turning str2 into a char[];
					stringPtr = new char[str1.size() +1]; 
		 
					copy(str1.begin(), str1.end(), stringPtr); 
					stringPtr[str1.size()] = '\0'; 
					
					argCount = 0; 			
					do{
						argPtr = strsep(&stringPtr, " "); 
						args[argCount++] = argPtr; 
					}while(argPtr); 

					//Execute command
					execvp(args[0], args); 
					
				}//else if < 
				else if(findStringChar(commandVec.at(i), '<')){
						
					vector<string> tempV; 
					tempV = pipeRedirection(commandVec.at(i), "<");	
					string str1, str2; 
					str1 = tempV.at(0); 
					str2 = tempV.at(1); 

					int ff = open(str2.c_str(), O_RDONLY, S_IRUSR | S_IWUSR); 
					dup2(ff, 0); 
					
					//str2 into char[]
					stringPtr = new char[str1.size() +1]; 
		 
					copy(str1.begin(), str1.end(), stringPtr); 
					stringPtr[str1.size()] = '\0'; 
					
					argCount = 0; 			
					do {
						argPtr = strsep(&stringPtr, " "); 
						args[argCount++] = argPtr; 
					}while(argPtr); 

					//Execute command
					execvp(args[0], args);

				}//else if ('cd') 
				else if(findSubStr(commandVec.at(i), "cd")){
					stringPtr = new char[commandVec.at(i).size() +1]; 
			 
					copy(commandVec.at(i).begin(), commandVec.at(i).end(), stringPtr); 
					stringPtr[commandVec.at(i).size()] = '\0'; 
					
					argCount = 0; 			
					do{
						argPtr = strsep(&stringPtr, " "); 
						args[argCount++] = argPtr; 
					}while(argPtr); 

					chdir(args[1]); 
				}//else if ('-t') 
				else if(findSubStr(commandVec.at(i), "-t")){
					if(promptUser){
						promptUser = false; 
					}
					else{
						promptUser = true; 
					}
				}//else (fork()) because piping 
				else{
						int sss = commandVec.at(i).size(); 
						//cout<<"here"<<endl; 
						 if(sss > 4){
							//cout<<"in size"<<endl; 
							if(findSubStr(commandVec.at(i), "echo")){
								commandVec.at(i).erase(remove(commandVec.at(i).begin(), commandVec.at(i).end(), '\"'), commandVec.at(i).end());
								commandVec.at(i).erase(remove(commandVec.at(i).begin(), commandVec.at(i).end(), '\''), commandVec.at(i).end());
							}
						 }

					int fds[2]; 
					pipe (fds); 

					stringPtr = new char[commandVec.at(i).size() +1]; 
			 
						copy(commandVec.at(i).begin(), commandVec.at(i).end(), stringPtr); 
						stringPtr[commandVec.at(i).size()] = '\0'; 
						
						argCount = 0; 			
						do{
							argPtr = strsep(&stringPtr, " "); 
							args[argCount++] = argPtr; 
						}while(argPtr); 

					if(fork() == 0){
						dup2(fds[1],1); 
						close(fds[0]); 
						execvp(args[0], args); 
						cout<<"user problem occurred"<<endl; 
						
					}else {
						wait(NULL); 
						close(fds[1]); 
						dup2(fds[0],0); 
					}


				}
				delete[] stringPtr; 

			}

			//Last commandVec execute Keep first 4 if's and then change else(fork() one up for non-piping
			int lastCommandIndex = commandVec.size() -1;
			if(isAmb) lastCommandIndex = lastCommandIndex -1; 
			
			
			
			if(findStringChar(commandVec.at(lastCommandIndex), '>')){
						vector<string> tempV; 
						tempV = pipeRedirection(commandVec.at(lastCommandIndex), ">");	
						string str1, str2; 
						str1 = tempV.at(0); 
						str2 = tempV.at(1); 

						int ff = open(str2.c_str(), O_CREAT|O_WRONLY, S_IRUSR | S_IWUSR); 
						dup2(ff, 1); 
						
						//str2 into char[]
						stringPtr = new char[str1.size() +1]; 
			 
						copy(str1.begin(), str1.end(), stringPtr); 
						stringPtr[str1.size()] = '\0'; 
						
						argCount = 0; 			
						do{
							argPtr = strsep(&stringPtr, " "); 
							args[argCount++] = argPtr; 
						}while(argPtr); 

						//Execute command
						execvp(args[0], args); 
						
			}
			else if(findStringChar(commandVec.at(lastCommandIndex), '<')) {
						vector<string> tempV; 
						tempV = pipeRedirection(commandVec.at(lastCommandIndex), "<");	
						string str1, str2; 
						str1 = tempV.at(0); 
						str2 = tempV.at(1); 


						int ff = open(str2.c_str(), O_RDONLY, S_IRUSR | S_IWUSR); 
						dup2(ff, 0); 
						
						//str2 into char[]
						stringPtr = new char[str1.size() +1]; 
			 
						copy(str1.begin(), str1.end(), stringPtr); 
						stringPtr[str1.size()] = '\0'; 
						
						argCount = 0; 	
						
						do {
							argPtr = strsep(&stringPtr, " "); 
							args[argCount++] = argPtr; 
						}
						while(argPtr); 

						//Execute command
						execvp(args[0], args);
			}
			else if(findSubStr(commandVec.at(lastCommandIndex), "cd")) {
				//CREATES A ZOMBIE PROCESS
					stringPtr = new char[commandVec.at(lastCommandIndex).size() +1]; 
			 
					copy(commandVec.at(lastCommandIndex).begin(), commandVec.at(lastCommandIndex).end(), stringPtr); 
					stringPtr[commandVec.at(lastCommandIndex).size()] = '\0'; 
					bool aa = true; 
					argCount = 0; 			
					do{
						if(aa == true){
							argPtr = strsep(&stringPtr, " "); 
							aa = false; 
						}else{
							argPtr = strsep(&stringPtr, ""); 
						}
						args[argCount++] = argPtr; 

					}
					while(argPtr); 

					chdir(args[1]); 
					//exit(0); 
			} 
			else if(findSubStr(commandVec.at(lastCommandIndex), "-t")){
					if(promptUser) {
						promptUser = false; 
					}
					else {
						promptUser = true; 
					}
			}
			else {
					int sss = commandVec.at(lastCommandIndex).size(); 
						//cout<<"here"<<endl; 
						 if(sss > 4) {
							//cout<<"in size"<<endl; 
							if(findSubStr(commandVec.at(lastCommandIndex), "echo")){
								commandVec.at(lastCommandIndex).erase(remove(commandVec.at(lastCommandIndex).begin(), commandVec.at(lastCommandIndex).end(), '\"'), commandVec.at(lastCommandIndex).end());
								commandVec.at(lastCommandIndex).erase(remove(commandVec.at(lastCommandIndex).begin(), commandVec.at(lastCommandIndex).end(), '\''), commandVec.at(lastCommandIndex).end());
							}
						 }

						stringPtr = new char[commandVec.at(lastCommandIndex).size() +1]; 	 
						copy(commandVec.at(lastCommandIndex).begin(), commandVec.at(lastCommandIndex).end(), stringPtr); 
						stringPtr[commandVec.at(lastCommandIndex).size()] = '\0'; 
						
						argCount = 0; 

						do {
							argPtr = strsep(&stringPtr, " "); 
							args[argCount++] = argPtr; 
							
						}
						
						while(argPtr); 
							execvp(args[0], args);
							exit(0); 
			}



		}
		//First fork else
		else {    
		
			if(!isAmb)   {
				wait(NULL); 
			}
			
		}
		
		delete[] stringPtr; 
		commandVec.erase(commandVec.begin(), commandVec.end());
		
	}
	
	while(userInput != "exit"); 
	cout<<"============ Shell has exited =============="<<endl; 
    exit(1);
    	return "\n shell has closed without any errors"; 
}




/*
vector<string> parse(string line) {
    
    int i = 0;
    for (char c : line) {
        if (c == ' ' && !singleQ && !doubleQ){
            parsed_line.push_back("");
            i++;
        }
        else if ((c == '|' || c == '<' || c == '>') && !singleQ && !doubleQ){
            parsed_line.push_back(string(1,c));
            i++;
            parsed_line.push_back("");
            i++;

    return remove_whitespace(parsed_line);
}




void pipeCommand(char** cmd1, char** cmd2){
    int fds[2];
    pipe(fds);
    //creating child process #1
    if (fork() == 0) {
        dup2(fds[0],STDIN_FILENO);
        close(fds[1]);
        close(fds[0]);
        //execute my seccond command.
        //creating child process #2
    if (fork() == 0) {
        //redirect stdout to fds[1] end of the pipe.
        dup2(fds[1],STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        // Execute the first command.
        execvp(cmd1[0],cmd1);
    }
    wait(NULL);
    execvp(cmd2[0],cmd2);
    }
    close(fds[1]);
    close(fds[0]);
    wait(NULL);
}
*/

int main(int argc, char** argv) {
    
    
    cout << shellExe()<<endl;
    
    
 
    /*
    while(true) {
    userPrompt();
    string inputline;
    getline (cin, inputline);
    
    if(inputline == string("exit")) {
        cout << "Ending shell process" << endl;
        break;
    }
    
    int pid = fork();
    if (pid == 0) {
        char* args[] = {(char*) inputline.c_str(),NULL};
        execvp (args[0],args);
    }
    else {
        wait(0);
        }
    }
    */
}
