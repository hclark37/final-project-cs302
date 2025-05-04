Our final project is a chatroom consisting of a server hosted on a localhost with a port with clients connected to that server. 
The server collects messages from clients and sends them back out to other clients on the server. 
The clients are able to log in with a username and an optional password, which will allow the client to send encrypted messages to those with the same password. 

This chatroom can allow for easy communication between clients on the same localhost.
This project was inspired by vim's messaging system on the hydra/tesla machines. 

To compile client:
g++ ./client.cpp -o client -lssl -lcrypto
To compile server
g++ ./server.cpp -o server
