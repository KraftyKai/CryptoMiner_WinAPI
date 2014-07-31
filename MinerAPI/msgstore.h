#ifndef __MSGSTORE_H_INCLUDED_
#define __MSGSTORE_H_INCLUDED_

class MsgStore {
private:
	std::string setting;
	std::string parameter;
	MsgStore *next;

	MsgStore() { next = nullptr; }

	void SetRequest(std::string sett, std::string param);

	void SetNext(MsgStore *assigned_next);

	std::string GetRequest();

	MsgStore *Next() { return next; }

	friend class MsgQueue;
};

class MsgQueue {
	MsgStore *head;
	MsgStore *tail;
public:
	MsgQueue();

	~MsgQueue();

	void Insert(char* sett, char* param);

	std::string Pop();

	std::string Peak();
};

#endif