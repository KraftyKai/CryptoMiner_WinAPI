#ifndef __BASEMINER_H_INCLUDED__
#define __BASEMINER_H_INCLUDED__

class BaseMiner {
public:
	BaseMiner();
	BaseMiner(char*, int);
	void Start(char *path);
	void SetPort(int apiPort);

	void SetHost(char *apiHostIP);
};

#endif