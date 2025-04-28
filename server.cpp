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
#include <fstream>
#include <deque>
#include "json.hpp"
//https://github.com/nlohmann/json
using json = nlohmann::json;

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

const string end_msg = "END";

void split_string(const string& received, string &username, string& message) {
	//split the message between the two : ; i don't know why i didn't use .find(). 
	for (size_t i = 0; i < received.length(); i++) {
		if (received[i] == ':') {
			username = received.substr(0, i);
			message = received.substr(i + 1);
			return;
		}
	}
	
	//if no user - i guess if somebody sent a super malformed packet by a custom client 
	message = received; // fallback 
	
	cerr << "ERROR: invalid message format!" << endl;
}

user_info* get_user(const string& username) {
	// search through users and return its struct if found 
	for (int i = 0; i < users.size(); i++) {
		if (users[i].username == username) {
			return &users[i];
		}
	}
	//add if doesnt exist 
	if (users.size() < max_users) {
		users.push_back({username, 0}); // should this set the time value?
		return &users.back();
	}
	
	return nullptr;
}

packet* add_packet(const string& msg, const char& flag) {
	string username, message; 
	//current time 
	time_t receive_time = time(NULL);
	//split up the message to its parts 
	split_string(msg, username, message);
	
	//if the message is empty, return a temporary packet because we won't be saving it. 
	if (message.empty()) {
		//cerr << "ERROR: message invalid!" << endl; //intended behavior since updates are with blank messages
		intermediate_packet = {username, msg, receive_time, false}; //this is the worst possible way to do this. global variable  
		return &intermediate_packet;
	}
	
	//packet replacement - there's a big queue of packets with a max size 
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
	//the encrypted messages aren't utf-8 so they make it crash so instead of trying to find a fix i'm just making them not get pushed to json. sorry not sorry!
	if (!new_packet.encrypted) {
		//open the file 
		ofstream f("log.json", std::ios::app);
		
		json j;
		//plug in values 
		j["user"] = new_packet.user;
	
		j["message"] = new_packet.message;
	
		j["receive_time"] = new_packet.receive_time;
	
		j["encrypted"] = new_packet.encrypted;
	
		//push json to file 
		f << j << endl;
	
		f.close();
	}
	
	return &packets.back();
}

int sock; //because it needs to be available in the signal handler 

//https://www.tutorialspoint.com/cplusplus/cpp_signal_handling.htm 
// this isn't really useful as i thought it would be 
void signal_handler(int signum) {
	cout << endl << "Received SIGINT (Ctrl + C). Closing the program..." << endl;
	
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

	ifstream f("log.json");
	
	if (f) {
		string line;
		deque<string> lines;
		//grab all the lines of the file 
		while(getline(f, line)) {
			lines.push_back(line);
			//if (lines.size() > 100) {
			//	lines.pop_front();
			//}
		}

		for (int i = 0; i < lines.size(); i++) {
			//take the line as a json 
			json j = json::parse(lines[i]);
			
			string user;
			string message; 
			time_t receive_time; 
			bool encrypted;
			//go through the json and grab all the values 
			for (json::iterator it = j.begin(); it != j.end(); ++it) {
				string key = it.key();
				string value = it.value().dump();
				
				if (key == "user") {
					user = value.substr(1, value.length() - 2);
				} else if (key == "message") {
					//for some reason you have to take a substring because they love to add "quotation marks to both sides like this" for no perceptable reason 
					message = value.substr(1, value.length() - 2);
				} else if (key == "receive_time") {
					int time = stoi(value);
					receive_time = time;
				} else if (key == "encrypted") {
					//this line is totally unnecessary bcause it will always be false because i wrote an exception above to prevent encrypted to being pushed because otherwise it crashes. 
					//just letting you know. 
					if (value == "false") {
						encrypted = false;
					} else {
						encrypted = true;
					}
				}
			}
			
			//add the packets-  you don't have to worry about the age of the messages from the file because of the way they're added in this method 
			packet new_packet = {user, message, receive_time, encrypted};
			packets.push_back(new_packet);
			
			//turn into readalbe time - the documentation for this is somewhere in the code 
			struct tm *time_info = localtime(&receive_time);
		
			char time_string[80];

			strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);
			//print the previous messages 
			cout << "[" << time_string << "] " << user << ": " << message << endl;
		}
		
		
		
	} else {
		cout << "Log file does not exist. Creating it..." << endl;
		ofstream outf("log.json");
		if (outf) {
			outf << "";
			outf.close();
		} else {
			cerr << "ERROR: cannot create log file!" << endl;
		}
	}
	
	f.close();

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
		
		//remove the flag from message string 
		received = received.substr(1);
		
		//convert to time string 
		packet* post = add_packet(received, flag);
		
		struct tm *time_info = localtime(&post->receive_time);
		
		char time_string[80];

		strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);

		user_info* user = get_user(post->user);
		
		//output message 
		cout << "[" << time_string << "] " << post->user << ": " << post->message << endl;
		//i didn't wanna fix the indentation that's why there's this if statement 
		if (true) { //only send those that are new since last message or 
			for (int i = 0; i < packets.size(); i++) {
				if (user->last_received_time <= packets[i].receive_time && user->username != packets[i].user) {
					string message = "";
					//set flag 
					if (packets[i].encrypted) {
						message += '1';
					} else {
						message += '0';
					}
					// create message
					message += to_string(packets[i].receive_time) + ":" + packets[i].user + ":" + packets[i].message;
					
					int sent = sendto(sock, message.c_str(), message.size(), 0, (struct sockaddr *)&from, fromlen);
					if (sent < 0) { 
						cerr << "ERROR: can't send packet!" << endl;
					}
				}
			}
		}
		
		if (user == nullptr) {
			//shouldn't happen 
		} else {
			//update last post time 
			user->last_received_time = post->receive_time;
		}
		
		//send end message 
		int sent = sendto(sock, end_msg.c_str(), end_msg.size(), 0, (struct sockaddr *)&from, fromlen);
		
		if (sent < 0) {
			cerr << "ERROR: can't send END message!" << endl;
		}		
	}
	
	close(sock);
	
	return 0;
}
