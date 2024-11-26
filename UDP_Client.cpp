#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main(int argc, char* argv[]) {
    // Start Winsock
    WSADATA data;
    WORD version = MAKEWORD(2, 2);
    int wsOk = WSAStartup(version, &data);

    if (wsOk != 0) {
        cout << "Can't start Winsock! Error code: " << wsOk << endl;
        return 1;
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(20000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);
    if (out == INVALID_SOCKET) {
        cout << "Error creating socket: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    //Send the username to the server
    string name;
    cout << "What is your name: ";
    getline(cin, name); //Use getline for input (to handle spaces, etc.)

    int sendOk = sendto(out, name.c_str(), name.size(), 0, (sockaddr*)&server, sizeof(server));
    if (sendOk == SOCKET_ERROR) {
        cout << "Error sending message to server: " << WSAGetLastError() << endl;
        closesocket(out);
        WSACleanup();
        return 1;
    }

    //Set up the socket to receive messages
    int serverLength = sizeof(server);
    char buf[1024];
    string message;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(out, &readfds); //Monitor the UDP socket

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000; //50ms timeout

        int activity = select(0, &readfds, NULL, NULL, &timeout);
        if (activity == SOCKET_ERROR) {
            cout << "Error in select: " << WSAGetLastError() << endl;
            continue;
        }

        //If there is data available on the UDP socket
        if (FD_ISSET(out, &readfds)) {
            int bytesIn = recvfrom(out, buf, 1024, 0, (sockaddr*)&server, &serverLength);
            if (bytesIn == SOCKET_ERROR) {
                cout << "Error receiving from server: " << WSAGetLastError() << endl;
                continue;
            }

            //Print the message received from the server
            cout << buf << endl;
        }

        //Check if the user typed something using _kbhit()
        if (_kbhit()) {
            char input = _getch();  //Read character from keyboard
            if (input == '\r' || input == '\n') {
                if (message.empty()) continue;  //Skip empty input

                //Send the message to the server
                sendOk = sendto(out, message.c_str(), message.size() + 1, 0, (sockaddr*)&server, sizeof(server));
                if (sendOk == SOCKET_ERROR) {
                    cout << "Error sending message to server: " << WSAGetLastError() << endl;
                    break;
                }

                if (message == "leave") break; //Exit if "leave" is entered
                message.clear();
                cout << endl; 
                cout.flush(); 
            }
            else if (input == '\b') { //Handle backspace
                if (!message.empty()) {
                    message.pop_back(); //Remove the last character from the buffer
                    cout << "\b \b"; //Move the cursor back and overwrite the character with a space
                }
            }
            else {
                message += input; //Accumulate the input characters
                cout << input; //Echo the character back to the console
            }
        }
    }

    //Close socket
    closesocket(out);
    //Close Winsock
    WSACleanup();

    return 0;
}
