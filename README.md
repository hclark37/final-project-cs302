Our final project is a chatroom consisting of a server hosted on a localhost with a port with clients connected to that server. 
The server collects messages from clients and sends them back out to other clients on the server. 

This chatroom can allow for easy communication between clients on the same localhost.
This project was inspired by vim's messaging system on the hydra/tesla machines. 

Features:   
Sockets are sent between clients and the server that carry the message, username, and time message was recieved. 
The clients are able to log in with a username and an optional password, which will allow the client to send encrypted messages to those with the same password. 
With the optional password, messages can be encrypted with a 8-byte sha512 encryption by starting the message with '%'.
Messages are limited to 1024 characters, or 256 when encrypted.
Messages are recieved to the client from the server when the client sends a message, including a blank message of which will not be recieved by the server.

To run: 
install client.cpp, server.cpp, json.hpp

To compile client:
g++ ./client.cpp -o client -lssl -lcrypto
To compile server
g++ ./server.cpp -o server

Project by Harrison Clark & Parker Dawes
