#ifndef _QUEUE_H_
#define _QUEUE_H_

template <typename T>
class Queue {
public:
	Queue();
	T pop();
	void push(T);
	bool is_empty();
	~Queue();
private:
	struct Node {
		T data;
		Node *next;
		Node(T);
	};

	Node *head, *tail;
};

template <typename T>
Queue<T>::Node::Node(T _data) : data(_data), next(nullptr) {

}

template<typename T>
Queue<T>::Queue() : head(nullptr), tail(nullptr) {

}

template<typename T>
bool Queue<T>::is_empty() {
	return head == nullptr;
}

template<typename T>
void Queue<T>::push(T data) {
	if (head == nullptr) {
		head = tail = new Node(data);
	} else {
		tail->next = new Node(data);
		tail = tail->next;
	}
}

template<typename T>
T Queue<T>::pop() {
	Node *node = head;
	head = head->next;
	if (head == nullptr) {
		tail = nullptr;
	}
	T retData = node->data;
	delete node;
	return retData;
}

template<typename T>
Queue<T>::~Queue() {
	while (head != nullptr) {
		Node *old = head;
		head = head->next;
		delete old;
	}
}

#endif