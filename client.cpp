#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>

//https://github.com/elidill653/RC4-Stream-Cipher/blob/master/RC4%20-Stream-Cipher.cpp

using namespace std;

//https://medium.com/@amit.kulkarni/encrypting-decrypting-a-file-using-openssl-evp-b26e0e4d28d4
//https://kolinsturt.github.io/lessons/2011/11/15/aes-encryption-using-openssl
//https://r00m.wordpress.com/2010/09/11/simple-xor-encryption-in-cpp/
string encrypt(const string& input, const string& password) {
	//uint_8 are unsigned char - needed for this 
	vector<uint8_t> data(input.begin(), input.end());
	vector<uint8_t> key(data.size());
	
	const uint8_t salt[] = {'a', 'b', 'c', 'd'};
	
	//makes your key legit 
	PKCS5_PBKDF2_HMAC(password.c_str(), password.size(), salt, sizeof(salt), 1000, EVP_sha512(), key.size(), key.data());
	
	for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= key[i];
    }
	
	return string(data.begin(), data.end());
}

void split_string(const string& received, time_t &time, string &username, string& message) {
	size_t first_colon = received.find(':');
	
	time = atoi(received.substr(0, first_colon).c_str());
	
	size_t second_colon = received.find(':', first_colon + 1);

	username = received.substr(first_colon + 1, second_colon - first_colon - 1);
	
	message = received.substr(second_colon + 1);
}


int main(int argc, char *argv[]) {
	
	if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <server> <port> <username> (optional)<password>" << endl;
        return 1;
    }
	
	string password = "";
	
	if (argc == 5) {
		password = argv[4];
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
			
			time_t time;
			
			split_string(response, time, username, message);
			
			struct tm *time_info = localtime(&time);
		
			char time_string[80];

			strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);

			if (username.empty()) { //shouldn't be possible but you never know ! 
				username = "Anonymous";
			}
			
			if (flag == '1' && argc == 5) {
				message = encrypt(message, password);
				if (message[0] != '%') {
					message = "<Encrypted message>";
				} else {
					message = message.substr(1);
				}
			} else if (flag == '1') {
				message = "<Encrypted message>";
			} 
			
			cout << "[" << time_string << "] " << username << ": " << message << endl;
			
        }
    }

    close(sock);
 
	return 0;
}
