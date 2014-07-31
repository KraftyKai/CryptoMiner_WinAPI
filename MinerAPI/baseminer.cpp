#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include "baseminer.h"

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL; 
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#pragma comment(lib, "User32.lib")

#define BUFFER_LEN 512
#define DEFAULT_POOL "http://api.bitcoin.cz:8332"
#define DEFAULT_POOL_USER "kraftykai.worker1"
#define DEFAULT_POOL_PASSWORD "test1"



BaseMiner::BaseMiner() {
	host = "127.0.0.1";
	port = 4028;
	poolURL = DEFAULT_POOL;
	poolUser = DEFAULT_POOL_USER;
	poolPassword = DEFAULT_POOL_PASSWORD;
}

BaseMiner::BaseMiner(char* hostIP, int apiPort, char* pool_url,
						char* pool_user, char* pool_password){
	if (hostIP == NULL)
		host = "127.0.0.1";
	else
		host = std::string(hostIP);

	if (pool_url == NULL)
		poolURL = DEFAULT_POOL;
	else
		poolURL = std::string(pool_url);

	if (pool_user == NULL)
		poolUser = DEFAULT_POOL_USER;
	else
		poolUser = std::string(pool_user);

	if (pool_password == NULL)
		poolPassword = DEFAULT_POOL_PASSWORD;
	else
		poolPassword = std::string(pool_password);

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

bool BaseMiner::Start(std::string path)
{
	DWORD result;
	if ((result = WaitForSingleObject(process.hProcess, 0)) != WAIT_OBJECT_0 && result != WAIT_FAILED)
	{
		throw std::runtime_error("Previous miner process is not terminated");
		return false;
	}

	std::string fullArgString = ConstructCommandLine(path);
	LPSTR fullArg = &fullArgString[0]; // Full Command Line Argument to pass
	SECURITY_ATTRIBUTES sa;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// Set pipe handles to inherited
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	
	// Create pipe for STDOUT
	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0))
		throw std::runtime_error("Could not create STDOUT pipe for child process");

	// Ensure handle not inherited
	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		throw std::runtime_error("Could not set STDOUT Handle information");

	// Create pipe for STDIN
	if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &sa, 0))
		throw std::runtime_error("Could not create STDIN pipe for child process");

	// Ensure handle not inherited
	if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
		throw std::runtime_error("Could not set STDIN Handle information");
	
	

	// Create the new [child] process

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	//Set up STARTUPINFO structure to pipe STDIN and STDOUT
	si.cb = sizeof(STARTUPINFO);
	si.hStdError = g_hChildStd_OUT_Wr;
	si.hStdOutput = g_hChildStd_OUT_Wr;
	si.hStdInput = g_hChildStd_IN_Rd;
	si.dwFlags = STARTF_USESTDHANDLES; // Required as it tells the starting process to use custom handles (I/O pipes)
	
	std::cout << std::endl << fullArg << std::endl;

	if (!CreateProcess(
		NULL,		// No module name (use purely command line)
		fullArg,	// Command line
		NULL,		// Process handle not inheritable
		NULL,		// Thread handle not inheritable
		TRUE,		// Set handle inheritance to TRUE
		CREATE_NEW_CONSOLE,			// No creation flags
		NULL,		// User parent's environment block
		"C:\\Users\\KraftyKai\\Documents\\Visual Studio 2013\\Projects\\Unit_Tests\\Unit_Tests\\SGMINER_BIN\\",		// User parent's starting directory
		&si,		// STARTUPINFO struct
		&pi)		// PROCESS_INFORMATION struct
		)
	{
		throw std::runtime_error("failed to create new process on miner startup");
		return false;
	}

	DWORD exit_code;
	GetExitCodeProcess(pi.hProcess, &exit_code);
	std::cout << std::endl << "Process " << pi.dwProcessId << " status is [" <<  exit_code << "]" << std::endl;
	


	//WaitForSingleObject(pi.hProcess, INFINITE);
	Sleep(100);

	process = pi;
	GetExitCodeProcess(pi.hProcess, &exit_code);
	std::cout << "Process " << pi.dwProcessId << " status is [" << exit_code << "]" << std::endl;
	return true;
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

std::string BaseMiner::Commit() {
	std::string send_msg, result;
	char recv_msg[RECV_BUFFER_LEN];
	char *recv_ptr = &recv_msg[0];

	send_msg = requests.Pop();
	if (!SendToMiner((char*)send_msg.c_str(), &recv_ptr, RECV_BUFFER_LEN))
	{
		throw std::runtime_error("Error Committing for Send To Miner");
		return NULL;
	}

	result = std::string(recv_msg);
	return result;

}

std::string BaseMiner::Stop()
{
	std::string response;
	requests.Insert("quit", NULL);
	std::cout << std::endl << "Committing action: " << requests.Peak() << std::endl;
	response = Commit();
	//void CheckStop();  //Waits for and validates process termination (blocking)
	//CloseHandle(process.hProcess);
	//CloseHandle(process.hThread);
	return response;
}

bool BaseMiner::SendToMiner(char *sendbuf, char **returnbuf, int returnbuflen)
{
	
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	int iResult;
	char recvbuf[BUFFER_LEN];
	int recvbuflen = BUFFER_LEN;

	//TODO: FIx memset, it's trying to write to protected memory!
	std::cout << std::endl << "Send buf is: " << std::string(sendbuf) << std::endl;
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
	
	//TODO: FIX THIS!!!
	//Create target socket address
	sockaddr_in addr;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host.c_str());
	addr.sin_family = AF_INET;
	std::cout << std::endl << "sockaddr_in test: Port = " << port << ", host = " << host << std::endl;
	// Connect to server
	iResult = connect(ConnectSocket, (sockaddr *)&addr, (int)sizeof(addr));
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

/*int main()
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

}*/

