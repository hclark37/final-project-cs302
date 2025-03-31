# https://www.geeksforgeeks.org/socket-programming-python/
import socket

# define server address and port
host = '127.0.0.1' 
port = 8080  

# create socket 
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# connect to server
client_socket.connect((host, port))

# message to send
message = "Test message."

# send the message
client_socket.send(message.encode('utf-8'))

# close the connection
client_socket.close()
