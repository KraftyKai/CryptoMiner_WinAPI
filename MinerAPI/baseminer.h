#ifndef __BASEMINER_H_INCLUDED__
#define __BASEMINER_H_INCLUDED__

#include "msgstore.h"

class BaseMiner {
private:
	int port;
	std::string host;
	MsgQueue *requests = new MsgQueue();
	PROCESS_INFORMATION process;

	virtual LPSTR ConstructCommandLine(char *path)
	{
		// COMPILER NOTE: Older compilers do not gaurantee contiguous assignment 
		// of std::string in memory.  BE AWARE!  Do NOT use char* type instead as
		// the calling method requires returned value be a mutable string. 
		std::string fullPath = path;
		fullPath += " --api-allow W:127.0.0.1";
		fullPath += "\0";
		std::cout << std::endl << "This is...  " << (LPSTR)&fullPath[0];
		return &fullPath[0];
	}

	void CheckStop();

public:
	BaseMiner();
	BaseMiner(char*, int);
	~BaseMiner() { delete(requests); }

	void SetPort(int apiPort);

	void SetHost(char *apiHostIP);

	/*
	** Send prepared JSON char buffer to miner API via TCP/IP
	** char *sendbuf = pointer to buffer to send on connect
	** char **returnbuf = pointer to pointer of char buffer for return msg
	** int returnbuflen = sizeof(**returnbuf)
	*/
	bool SendToMiner(char *sendbuf, char **returnbuf, int returnbuflen);

	virtual void ConfigPool(char *serverURL, char *username,
		char *password, int port)
	{
		if (serverURL == NULL)
			return;

		//Prepare parameters in format "URL:PORT,USR,PASS
		std::string param;
		param = serverURL;
		param += ":";
		param += std::to_string(port);
		param += ",";
		param += username;
		param += ",";
		param += password;

		requests->Insert("addpool", (char*)param.c_str());
		//TODO Add commit method call.
	}

	bool Start(char *path);

	virtual void Stop()
	{
		requests->Insert("quit", NULL);
		//TODO Add commit method call
		void CheckStop();  //Waits for and validates process termination (blocking)
		CloseHandle(process.hProcess);
		CloseHandle(process.hThread);
	}

	virtual void Suspend() { return; }

	virtual void GetHashRate(){ return; }


};

#endif