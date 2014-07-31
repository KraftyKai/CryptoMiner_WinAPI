#ifndef __BASEMINER_H_INCLUDED__
#define __BASEMINER_H_INCLUDED__

#include "msgstore.h"

#define RECV_BUFFER_LEN 1000

class BaseMiner {
private:
	int port;
	std::string host, poolURL, poolUser, poolPassword;
	MsgQueue requests = MsgQueue();
	PROCESS_INFORMATION process;

	virtual std::string ConstructCommandLine(std::string& path)
	{
		// COMPILER NOTE: Older compilers do not gaurantee contiguous assignment 
		// of std::string in memory.  BE AWARE!  Do NOT use char* type instead as
		// the calling method requires returned value be a mutable string. 
		std::string fullPath = path;
		fullPath += " --api-listen --api-allow W:127.0.0.1 -o  \"";
		fullPath += poolURL;
		fullPath += "\" -u \"";
		fullPath += poolUser;
		fullPath += "\" -p \"";
		fullPath += poolPassword;
		fullPath += "\"\0";
		return fullPath;
	}

	bool SendToMiner(char *sendbuf, char **returnbuf, int returnbuflen);

	void CheckStop();
	
	/*
	** Send prepared JSON char buffer to miner API via TCP/IP
	** char *sendbuf = pointer to buffer to send on connect
	** char **returnbuf = pointer to pointer of char buffer for return msg
	** int returnbuflen = sizeof(**returnbuf)
	*/
	

public:
	BaseMiner();
	BaseMiner(char* hostIP, int, char* pool_url, char* pool_user, char* pool_password);
	~BaseMiner() {  }

	void SetPort(int apiPort);

	void SetHost(char *apiHostIP);

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

		requests.Insert("addpool", (char*)param.c_str());
		//TODO Add commit method call.
	}

	bool Start(std::string path);

	//TODO : Fix being an idiot about virtual functions
	virtual std::string Stop();

	virtual void Suspend() { return; }

	virtual void GetHashRate(){ return; }

	std::string Commit();

	std::string GetHost() { return host; }

	int GetPort() { return port; }

};

#endif