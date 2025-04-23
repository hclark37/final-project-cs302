#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//https://github.com/elidill653/RC4-Stream-Cipher/blob/master/RC4%20-Stream-Cipher.cpp

using namespace std;

string encrypt(string& input, string& password) {
	return input;
}

string decrypt(string& input, string& password) {
	return input;
}

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


int main(int argc, char *argv[]) {
	
	if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <server> <port> <username> (optional)<password>" << endl;
        return 1;
    }
	string password = "";
	
	if (argc == 5) {
		string password = argv[4];
	}
	
    string server_name = argv[1];
	
    int port = atoi(argv[2]);
    
	string username = argv[3];

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
	
    if (sock < 0) {
        cerr << "ERROR: can't open socket!" << endl;
		return 1;
    }

    struct sockaddr_in server_addr;
    struct hostent *server = gethostbyname(server_name.c_str());
	
    if (server == nullptr) {
        cerr << "ERROR: host not found: " << server_name << endl;
        return 1;
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);

    char buffer[1024];
    socklen_t server_len = sizeof(server_addr);

    while (true) {
        string message_content;
        cout << "Enter your message: ";
        getline(cin, message_content);
		char encryption_flag = '0';
		
		if (!message_content.empty()) {
			if (message_content[0] == '%') {
				encryption_flag = '1';
				message_content = message_content.substr(1);
			}
		}
		
		if (encryption_flag == '1' && argc == 5) {
			message_content = encrypt(message_content, password);
		} else if (encryption_flag == '1') {
			cerr << "ERROR: can't encrypt without password!" << endl;
		}
		
		string full_message = encryption_flag + username + ":" + message_content;
		
        int n = sendto(sock, full_message.c_str(), full_message.length(), 0, (struct sockaddr *)&server_addr, server_len);
		
		if (n < 0) {
            cerr << "ERROR: can't send!" << endl;
			return 1;
        }

        cout << "Waiting for server response(s)..." << endl;
		
        while (true) {
            n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &server_len);
            
			if (n < 0) {
                cerr << "ERROR: can't receive!" << endl; 
				return 1;
			}
            
			buffer[n] = '\0';  
			
			string response = buffer;
			
			if (response.empty() || response == "END") {
                break;
			}
			
			char flag = buffer[0]; //encryption flag 
			
			response = response.substr(1);
			
            string username, message;
				
			split_string(response, username, message);
				
			if (username.empty()) {
				username = "Anonymous";
			}
			if (flag == '1' && argc == 5) {
				message = decrypt(message, password);
			} else if (flag == '1') {
				message = "<Encrypted message>";
			} 
				
			cout << "Received: " << username << ": " << message << endl;
			
        }
    }

    close(sock);
 
	return 0;
}
