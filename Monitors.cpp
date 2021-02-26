#include "monitor.h"
#include <stdio.h>
#include <queue>
#include <pthread.h>
#include <string>
#include <iostream>
#include <time.h>
#include <vector>
#include <math.h>


using namespace std;



class Message
{
public:
	string mess;
	time_t time1;
	time_t time2;

	Message(string m) {
		mess = m;
	}
};


class Queue : public Monitor
{
public:
	deque<Message> q;
	int q_length;
	int placeforvip;
	Condition NotFull;
	Condition NotEmpty;

	Queue(int len) {
		this->q_length = len;
		this->placeforvip = 0;
	}
	
	void put_user(Message mess) {
		if (mess.mess.length() > 16 || mess.mess.length() < 8) {
			cout << "Wrong message size in message: " << mess.mess << endl;
			return;
		}
		enter();
		mess.time1 = time(0);
		if (q.size() == q_length) {
			wait(NotFull);
		}
		mess.time2 = time(0);
		q.push_back(mess);
		cout << "PUT MESSAGE: " << mess.time1 << " " << mess.time2 << " " << mess.mess << endl;
		this->show_q();
		if (q.size() == 1) {
			signal(NotEmpty);
		}
		
		leave();
	}

	void put_vip(Message mess) {
		if (mess.mess.length() > 16 || mess.mess.length() < 8) {
			cout << "Wrong message size in message: " << mess.mess << endl;
			return;
		}
		enter();
		mess.time1 = time(0);
		if (q.size() == this->q_length) {
			wait(NotFull);
		}
		mess.time2 = time(0);
		if (q.size() != 0){
			q.push_back(q[q.size() - 1]);
			for (int i = q.size()-2; i >= placeforvip; i--) {
				q[i + 1] = q[i];
			}
		q[placeforvip] = mess;
		}
		else{
		q.push_back(mess);
		}
		
		placeforvip++;
		cout << "PUT MESSAGE: " << mess.time1 << " " << mess.time2 << " " << mess.mess << endl;
		this->show_q();
		if (q.size() == 1) {
			signal(NotEmpty);
		}
		
		leave();
	}

	void read() {
		enter();
		time_t time1 = time(0);
		if(q.size() == 0) {
			wait(NotEmpty);
		}
		time_t time2 = time(0);
		cout << "READING TIMES: " << time1 << ", " << time2 << endl << "MESSAGE: " <<  q[0].time1 << " " << q[0].time2 << " " << q[0].mess << endl;
		q.pop_front();
		if (placeforvip > 0){
			placeforvip--;
		}
		this->show_q();
		if (q.size() == this->q_length -1) {
			signal(NotFull);
		}
		
		leave();
	}

	void show_q() {
		cout << "\t\t\t\t\t\tQUEUE:" << endl;
		for (int i = 0; i < q.size(); i++ ) {
			cout << "\t\t\t\t\t\t"<< i << ". " << q[i].time1 << " " << q[i].time2 << " " << q[i].mess << endl;
		}
	}



};

Queue QUEUE = Queue(5);

//TEST 1 - full queue
/*
int vip_times[4] = { 0,1,1,1 };
int user_times[4] = { 0,1,1,1 };
int reader_times[4] = { 8,1,1,1};
*/
//TEST 2 - empty queue
/*
int vip_times[4] = { 5,1,1,1 };
int user_times[4] = { 5,1,1,1 };
int reader_times[4] = {0,1,1,1};
*/
// TEST 3 n-1 user, last vip
/*
int vip_times[4] = { 4,1,1,1};
int user_times[4] = { 0,1,1,1 };
int reader_times[3] = { 5,1,1};
*/
//TEST 4 n-1 vip, last user
/*
int vip_times[] = { 0,1,1,1 };
int user_times[] = { 4,1,1,1 };
int reader_times[] = { 5,1,1 };
*/

int vip_iterator = 0;
int user_iterator = 0;
int reader_iterator = 0;

void* vip(void* args) {
	for(;;){
	
	if (vip_iterator >= sizeof(vip_times)/sizeof(vip_times[0])) {
		return NULL;
	}
	usleep(vip_times[vip_iterator]*pow(10,6));
	vip_iterator++;
	string m = "vvvvvvv " + to_string(vip_iterator);
	QUEUE.put_vip(Message(m));
}
}

void* user(void* args) {
	for(;;){
	
	if (user_iterator >= sizeof(user_times) / sizeof(user_times[0])) {
		return NULL;
	}
	usleep(user_times[user_iterator]*pow(10,6));
	user_iterator++;
	string m = "uuuuuuu " + to_string(user_iterator);
	QUEUE.put_user(Message(m));
}
}

void* reader(void* args) {
	for(;;){
	if (reader_iterator >= sizeof(reader_times) / sizeof(reader_times[0])) {
		return NULL;
	}
	usleep(reader_times[reader_iterator]*pow(10,6));
	reader_iterator++;
	QUEUE.read();
}
}



int main() {	
	pthread_t  thread_vip, thread_usr, thread_read;
	pthread_create(&thread_read, NULL, &reader, NULL);

	pthread_create(&thread_usr, NULL, &user, NULL);
	pthread_create(&thread_vip, NULL, &vip, NULL);

	pthread_join(thread_read, NULL);

	pthread_join(thread_usr, NULL);
	pthread_join(thread_vip, NULL);
	
	//QUEUE.put_vip(Message("a"));
	//QUEUE.put_vip(Message("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));



}
