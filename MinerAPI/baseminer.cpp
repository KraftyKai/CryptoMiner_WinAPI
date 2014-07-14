#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <string>

#include "msgstore.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define BUFFER_LEN 512

class BaseMiner {
private:
	int port;
	std::string host;
	MsgQueue *requests = new MsgQueue();
	PROCESS_INFORMATION process;
	
	virtual LPSTR ConstructCommandLine( char *path) 
	{
		// COMPILER NOTE: Older compilers do not gaurantee contiguous assignment 
		// of std::string in memory.  BE AWARE!  Do NOT use char* type instead as
		// the calling method requires returned value be a mutable string. 
		std::string fullPath = path;
		fullPath += " --api-allow W:127.0.0.1";
		fullPath += "\0";
		return &fullPath[0];
	}

	void CheckStop();

public:
	BaseMiner();
	BaseMiner(char*, int);

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

	void Start(char *path);

	virtual void Stop() 
	{ 
		requests->Insert("quit", NULL);
		//TODO Add commit method call
		void CheckStop();  //Waits for and validates process termination (blocking)
		CloseHandle(process.hProcess);
		CloseHandle(process.hThread);
	}

	virtual void Suspend() { return; }

	virtual void GetHashRate() { return; }


};

BaseMiner::BaseMiner() {
	host = "127.0.0.1";
	port = 4028;
}

BaseMiner::BaseMiner(char *hostIP, int apiPort){
	if (hostIP == NULL)
		host = "127.0.0.1";
	else
		host = hostIP;

	port = apiPort;
}

void BaseMiner::SetPort(int apiPort)
{
	port = apiPort;
}

void BaseMiner::SetHost(char *apiHostIP)
{
	host = apiHostIP;
}

void BaseMiner::Start(char *path)
{
	LPDWORD exitCode;
	DWORD result;
	if ((result = WaitForSingleObject(process.hProcess, 0)) != WAIT_OBJECT_0 && result != WAIT_FAILED)
	{
		throw std::runtime_error("Previous miner process is not terminated");
		return;
	}
	
	LPSTR fullArg = ConstructCommandLine(path); // Full Command Line Argument to pass

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(
		NULL,		// No module name (use purely command line)
		fullArg,	// Command line
		NULL,		// Process handle not inheritable
		NULL,		// Thread handle not inheritable
		FALSE,		// Set handle inheritance to FALSE
		0,			// No creation flags
		NULL,		// User parent's environment block
		NULL,		// User parent's starting directory
		&si,		// STARTUPINFO struct
		&pi)		// PROCESS_INFORMATION struct
		)
	{
		throw std::runtime_error("failed to create new process on miner startup");
		return;
	}

	process = pi;
}

void BaseMiner::CheckStop()
{
	DWORD result;
	//If application is still open...
	if ((result = WaitForSingleObject(process.hProcess, 0)) == WAIT_TIMEOUT)
	{
		// Wait too see if application closes for up to 90 seconds. 
		for (int i = 0; i < 10 && (result = WaitForSingleObject(process.hProcess, 90000)) == WAIT_TIMEOUT; i++);
	}

	// If it's STILL open try to force termination.  If after another 90s wait still failing, throw exception
	if (result == WAIT_TIMEOUT)
	{
		TerminateProcess(process.hProcess, 0);

		if (WaitForSingleObject(process.hProcess, 90000) == WAIT_TIMEOUT)
			throw std::runtime_error("Miner application failed to stop");
	}
	return;
}
bool BaseMiner::SendToMiner(char *sendbuf, char **returnbuf, int returnbuflen)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	int iResult;
	char recvbuf[BUFFER_LEN];
	int recvbuflen = BUFFER_LEN;

	memset(*returnbuf, 0, returnbuflen - 1);
	//Initialize Winsock
	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
		return false;

	// Create a socket for connecting to server
	ConnectSocket = socket(AF_INET, SOCK_STREAM,
		IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET)
	{
		throw std::runtime_error("failed to create socket");
		return false;
	}

	//Create target socket address
	sockaddr_in addr;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host.c_str());
	addr.sin_family = AF_INET;

	// Connect to server
	iResult = connect(ConnectSocket, (sockaddr *)&addr, sizeof(addr));
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		WSACleanup();
		throw std::runtime_error("failed to connect to miner");
		return false;
	}

	// Send Payload
	for (int i = 0; sendbuf != NULL && i < (int)strlen(sendbuf);)
	{
		char *sbuf = &sendbuf[i];
		int len = (int)strlen(sbuf);
		iResult = send(ConnectSocket, sendbuf, len, 0);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			WSACleanup();
			throw std::runtime_error("failed to send payload over socket");
			return false;
		}
		i += iResult;
	}

	//shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		WSACleanup();
		throw std::runtime_error("failed to shutdown socket after send");
	}

	// Receive until peer closes connection
	int i = 0;
	do {

		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);

		if (iResult > 0 && (i + iResult) < returnbuflen)
		{
			memcpy(&(*returnbuf[i]), recvbuf, iResult);
			i += iResult;
		}
		else if (iResult == 0)
			break;
		else if (iResult < 0)
		{
			closesocket(ConnectSocket);
			WSACleanup();
			throw std::runtime_error("socket recv error");
			return false;
		}
		else
		{
			closesocket(ConnectSocket);
			WSACleanup();
			throw std::runtime_error("buffer overflow detected");
			return false;
		}

	} while (iResult > 0);

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();
	return true;
}




//Main function for testing only

int main()
{
  int const msg_size = 2000;
  BaseMiner miner ("10.0.0.10", 3490);
  char send[] = "Hello\0";
  char msg[msg_size];
  bool result;
  int port = 3490;

  char *sendptr = send;
  char *msgptr = msg;
 // try{
    result = miner.SendToMiner( NULL, &msgptr, msg_size);
  //}

 // catch (std::exception& e) {
    //std::cout << e.what();
  //}

  if (result)
	std::cout << msg << std::endl;

}

