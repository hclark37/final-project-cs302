#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal> // for clean killing it with ctrl + c

// https://www.linuxhowtos.org/C_C++/socket.htm

using namespace std;

const int max_packets = 100;
const int max_users = 100;
int total_packets_added = 0;

struct user_info {
	string username;
	time_t last_received_time = 0;
};

struct packet {
	string user;
	string message;
	time_t receive_time;
	bool encrypted;
};

vector<user_info> users;
vector<packet> packets;
packet intermediate_packet;

void split_string(const string& received, string &username, string& message) {
	for (size_t i = 0; i < received.length(); i++) {
		if (received[i] == ':') {
			username = received.substr(0, i);
			message = received.substr(i + 1);
			return;
		}
	}
	
	message = received; // fallback 
	
	cerr << "ERROR: invalid message format!" << endl;
}

user_info* get_user(const string& username) {
	for (int i = 0; i < users.size(); i++) {
		if (users[i].username == username) {
			return &users[i];
		}
	}
	
	if (users.size() < max_users) {
		users.push_back({username, 0}); // should this set the time value?
		return &users.back();
	}
	
	return nullptr;
}

packet* add_packet(const string& msg, const char& flag) {
	if (total_packets_added >= max_packets) {
		//PARKER, you can add a function call here to push everything to a json file.
		//not sure if this conditional statement is right
		total_packets_added = 0;
	}
	
	string username, message; 
	time_t receive_time = time(NULL);
	
	split_string(msg, username, message);
	
	if (message.empty()) {
		//cerr << "ERROR: message invalid!" << endl; //intended behavior since updates are with blank messages
		intermediate_packet = {username, msg, receive_time, false}; //this is the worst possible way to do this. 
		return &intermediate_packet;
	}
	
	if (packets.size() == max_packets) {
		packets.erase(packets.begin());
	}
	
	packet new_packet;
	
	if (flag == '1') {
		new_packet = {username, message, receive_time, true};
	} else {
		new_packet = {username, message, receive_time, false};
	}
	packets.push_back(new_packet);
	
	total_packets_added += 1; //iterator to see when you've fully looped through the vector
	
	return &packets.back();
}

int sock;

//https://www.tutorialspoint.com/cplusplus/cpp_signal_handling.htm 
void signal_handler(int signum) {
	cout << endl << "Received SIGINT (Ctrl + C). Saving messages..." << endl;
	
	//parker, this is another place where you're gonna run your save to json part. 
	//you're only going to want to save the ones since the last save in the program. you'll have to do the math on that
	//based upon the total_packets_added part of the code
	
	close(sock);
	
	exit(0);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_handler);

	if (argc < 2) {
		cerr << "ERROR: no port provided!" << endl;
		return 1;
	}
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (sock < 0) { 
		cerr << "ERROR: can't open socket!" << endl;
		return 1;
	}
	
	struct sockaddr_in server;
	
	server.sin_family = AF_INET;

	server.sin_addr.s_addr = INADDR_ANY;

	server.sin_port = htons(stoi(argv[1]));
	
	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		cerr << "ERROR: couldn't bind!" << endl;
		return 1;
	}
	
	struct sockaddr_in from;

	socklen_t fromlen = sizeof(from);
	
	char buffer[1024];

	const string end_msg = "END";

	while (true) {
				
		int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&from, &fromlen);
		
		if (n < 0) {
			cerr << "ERROR: problem receiving from socket!" << endl;
			continue; //how to handle this? 
		}
		
		if (n < 2) { 
			cerr << "ERROR: message too short!" << endl;
			continue;
		}
		
		char flag = buffer[0]; //encryption flag 
		
		buffer[n] = '\0'; //null terminate string 
		
        string received = buffer;
		
		received = received.substr(1);
		
		packet* post = add_packet(received, flag);
		
		user_info* user = get_user(post->user);
		
		cout << "Datagram received: " << received << endl;
		
		if (user == nullptr) { //send all if undefined user 
			//cout << "User is nullptr!" << endl; // debug 
			for (int i = 0; i < packets.size(); i++) {
				int sent = sendto(sock, packets[i].message.c_str(), packets[i].message.size(), 0, (struct sockaddr *)&from, fromlen);
				if (sent < 0) { 
					cerr << "ERROR: can't send packet!" << endl;
				}
			}
		} else { //only send those that are new since last message or 
			for (int i = 0; i < packets.size(); i++) {
				if (user->last_received_time <= packets[i].receive_time && user->username != packets[i].user) {
					string message;
					if (packets[i].encrypted) {
						message += '1';
					} else {
						message += '0';
					}
					message += packets[i].user + ":" + packets[i].message;
					int sent = sendto(sock, message.c_str(), message.size(), 0, (struct sockaddr *)&from, fromlen);
					if (sent < 0) { 
						cerr << "ERROR: can't send packet!" << endl;
					}
				}
			}
		}
		
		if (user == nullptr) {
			
		} else {
			user->last_received_time = post->receive_time;
		}
		
		int sent = sendto(sock, end_msg.c_str(), end_msg.size(), 0, (struct sockaddr *)&from, fromlen);
		
		if (sent < 0) {
			cerr << "ERROR: can't send END message!" << endl;
		}		
	}
	
	close(sock);
	
	return 0;
}
