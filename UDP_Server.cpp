#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    //Start Winsock
    WSADATA data;
    WORD version = MAKEWORD(2, 2);
    int wsOk = WSAStartup(version, &data);

    if (wsOk != 0) {
        cout << "Can't Start Winsock! Error code: " << wsOk << endl;
        return 1;
    }

    SOCKET inSocket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serverHint;
    serverHint.sin_addr.s_addr = ADDR_ANY;
    serverHint.sin_family = AF_INET;
    serverHint.sin_port = htons(20000);

    //If can't bind socket
    if (bind(inSocket, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR) {
        cout << "Can't bind socket! Error: " << WSAGetLastError() << endl;
        return 1;
    }

    cout << "The server is now running on port number 20000\n";

    sockaddr_in client;
    int clientLength = sizeof(client);
    ZeroMemory(&client, clientLength);

    char buf[1024];
    unordered_map<string, pair<string, sockaddr_in>> allUsers; //Store users with clientID, username, and address

    while (true) {
        ZeroMemory(buf, 1024);

        //Wait for message from client
        int bytesIn = recvfrom(inSocket, buf, 1024, 0, (sockaddr*)&client, &clientLength);
        if (bytesIn == SOCKET_ERROR) {
            int error = WSAGetLastError();
            //Ignore certain errors caused by client disconnection
            if (error == WSAECONNRESET || error == WSAEINTR) {
                continue; //Skip this iteration; the client has disconnected
            }
            cout << "Error receiving from client: " << error << endl;
            continue;
        }

        //Display client information
        char clientIP[256];
        ZeroMemory(clientIP, 256);
        inet_ntop(AF_INET, &client.sin_addr, clientIP, sizeof(clientIP));

        string message(buf);
        string clientID = to_string(client.sin_addr.S_un.S_addr) + ":" + to_string(client.sin_port);
        
        //All commands user can use
        if (allUsers.find(clientID) == allUsers.end()) {
            //New user joins the chat
            cout << message << " has joined the chat!" << endl;
            allUsers[clientID] = make_pair(message, client);
        }
        else if (message == "users") { //Command to list users
            string userList = "Users online: ";
            for (const auto& pair : allUsers) {
                userList += pair.second.first + " "; //Get the username
            }

            sendto(inSocket, userList.c_str(), userList.size() + 1, 0, (sockaddr*)&client, sizeof(client));
        }
        else if (message.substr(0, 7) == "to all ") { //Send message to all online users
            string msg = message.substr(7);

            //Build the message to broadcast
            string fullMessage = allUsers[clientID].first + " says to everyone: <" + msg + ">";

            //Send the message to every connected user except the sender
            for (const auto& pair : allUsers) {
                if (pair.first != clientID) {
                    sockaddr_in recipientAddr = pair.second.second; //Get recipient's address
                    sendto(inSocket, fullMessage.c_str(), fullMessage.size() + 1, 0, (sockaddr*)&recipientAddr, sizeof(recipientAddr));
                }
            }
        }
        else if (message.substr(0, 3) == "to ") { //Command to send message to specific users
            size_t msgIndex = message.find(" msg ");
            if (msgIndex != string::npos) {
                string recipients = message.substr(3, msgIndex - 3);
                string msg = message.substr(msgIndex + 5);

                //Split recipients into individual usernames
                stringstream ss(recipients);
                string username;
                vector<string> usernames;

                while (ss >> username) {
                    usernames.push_back(username);
                }

                //Send the message to each recipient
                for (const string& recipient : usernames) {
                    bool foundRecipient = false;

                    for (const auto& pair : allUsers) {
                        if (pair.second.first == recipient) { //Found the recipient
                            foundRecipient = true;
                            sockaddr_in recipientAddr = pair.second.second; //Get recipient's address

                            string fullMessage = allUsers[clientID].first + " says: <" + msg + ">";
                            sendto(inSocket, fullMessage.c_str(), fullMessage.size() + 1, 0, (sockaddr*)&recipientAddr, sizeof(recipientAddr));
                        }
                    }

                    if (!foundRecipient) {
                        string errorMsg = "Error message:  " + recipient + " is not online!";
                        sendto(inSocket, errorMsg.c_str(), errorMsg.size() + 1, 0, (sockaddr*)&client, sizeof(client));
                    }
                }
            }
            else {
                string errorMsg = "Invalid 'to' command format.";
                sendto(inSocket, errorMsg.c_str(), errorMsg.size() + 1, 0, (sockaddr*)&client, sizeof(client));
            }
        }
        else if (message.substr(0, 5) == "leave") { //Command to leave chat
            cout << allUsers[clientID].first << " has left the chat!" << endl;
            allUsers.erase(clientID);
            string leaveMessage = allUsers[clientID].first + " has left the chat!";
            sendto(inSocket, leaveMessage.c_str(), leaveMessage.size() + 1, 0, (sockaddr*)&client, sizeof(client));
        }
        else { //Any other commands entered
            string mes = "Unknown command";
            sendto(inSocket, mes.c_str(), mes.size() + 1, 0, (sockaddr*)&client, sizeof(client));
        }
    }

    //Close socket
    closesocket(inSocket);
    //Close Winsock
    WSACleanup();

    return 0;
}
