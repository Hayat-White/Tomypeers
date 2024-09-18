# Tomypeers


This program functions to allow users to connect and send files. Current functioning version allows for, with port forwarding, users to send files of any size across multiple networks.
Built entirely on C and uses the WinSock2.h to build sockets for connections. Refactored and readable code inside directory Tomypeers/capstone/Projectworksolo/restructored code/. For program
to function properly two directories are required to be inside the directory the executable is inside of. A directory for downloaded files labeled download and a directory for uploaded files named
upload. All file names inside the download folder will be sent to the connected peer as well as the number of bytes those files currently occupy. If another peer connects to your host, the host is able
to connect to a peer that is currently not selected. Currently working towards implementing a meaningful security and uPnP to eliminate requirement for multi-network communication. Port can be changed,
default port numbers are 12172.
