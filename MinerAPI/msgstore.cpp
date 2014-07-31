#include <string>
#include "msgstore.h"

//Begin MsgStore class methods

void MsgStore::SetRequest(std::string sett, std::string param) 
{
	setting = sett;
	parameter = param;
}

void MsgStore::SetNext(MsgStore *assigned_next) 
{
	next = assigned_next;
}

std::string MsgStore::GetRequest()
{
	std::string result = "{\"command\":\"" + setting + "\"";
	if (!parameter.empty())
		result += ", \"parameter\":\"" + parameter + "\"";
	result += "}";
	return result;
}

// Begin MsgQueue class methods

MsgQueue::MsgQueue() 
{
	head = nullptr;
	tail = nullptr;
}

MsgQueue::~MsgQueue() {
	while (head != nullptr)
	{
		MsgStore *temp = head;
		head = head->Next();
		delete(temp);
	}
}

void MsgQueue::Insert(char* sett, char* param)
{
	MsgStore *newStore = new MsgStore();

	if (sett == nullptr)
		throw std::runtime_error("MsgQueue::Insert Null setting was passed.");

	if (param == nullptr)
		newStore->SetRequest(std::string(sett), "");
	else
		newStore->SetRequest(std::string(sett), std::string(param));

	if (head == nullptr) {
		head = newStore;
		tail = newStore;
	}
	else {
		tail->SetNext(newStore);
		tail = newStore;
	}
}

std::string MsgQueue::Pop()
{
	std::string result = head->GetRequest();
	if (head == nullptr)
		return result;

	MsgStore *temp = head;
	head = head->next;
	delete(temp);
	return result;
}

std::string MsgQueue::Peak() { return head->GetRequest(); }