#include <string>

class MsgStore {
private:
	std::string setting;
	std::string parameter;
	MsgStore *next;

	MsgStore() {
		next = NULL;
	}
	void SetRequest(char *sett, char *param) {
		setting = sett;
		parameter = param;
	}
	void SetNext(MsgStore *assigned_next) {
		next = assigned_next; 
	}
	const char *GetRequest() {
		std::string result = "{\"command\":\"" + setting + "\"";
		if (!parameter.empty())
			result += ", \"parameter\":\"" + parameter + "\"";
		result += "}";
		return result.c_str();
	}
	MsgStore *Next() { return next; }
	friend class MsgQueue;
};

class MsgQueue {
	MsgStore *head;
	MsgStore *tail;
public:
	MsgQueue() {
		head = NULL;
		tail = NULL;
	}
	~MsgQueue() {
		while (head != NULL)
		{
			MsgStore *temp = head;
			head = head->Next();
			delete(temp);
		}
	}

	void Insert(char*, char*);

	std::string Pop();

	std::string Peak();


};

void MsgQueue::Insert(char *sett, char *param)
{
	MsgStore *newStore = new MsgStore();
	newStore->SetRequest(sett, param);

	if (head == NULL) {
		head = newStore;
	}
	else {
		tail->SetNext(newStore);
		tail = newStore;
	}
}

std::string MsgQueue::Pop()
{
	std::string result = head->GetRequest();
	MsgStore *temp = head->Next();
	head = temp;
	delete(temp);
	return result;
}

std::string MsgQueue::Peak() { return head->GetRequest(); }