#include<WinSock2.h>
#include<WS2tcpip.h>
#include<iostream>
#include<string>
#include<cstdbool>

using namespace std;

const char SERVER_ADDRESS[] = "127.0.0.1";
const int SERVER_PORT = 5000;
const int BUFFER_SIZE = 2048;
const char MY_ERROR[] = "Error!";
const char USER_NOT_FOUND[] = "User not found!";
const char WRONG_PASSWORD[] = "You have typed wrong password. Please retry!";
const char ASK_PASSWORD[] = "Password: ";
const char SUCCESS[] = "Success!";

typedef struct
{
	char type[5];
	char payload[1000];
} message;

const char MESSAGE_USER[] = "USER";
const char MESSAGE_PASS[] = "PASS";
const char MESSAGE_LOGOUT[] = "LOUT";

const int MESSAGE_SIZE = sizeof(message);

int main(int argc, char* argv[])
{
	if (argc < 5)
	{
		cerr << "Missing arguments!" << endl;

		return 1;
	}

	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
	{
		cerr << "Version is not supported!" << endl;
	}

	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	int timeout = 10000;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(int));

	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((u_short) SERVER_PORT);
	if (inet_pton(AF_INET, SERVER_ADDRESS, (void*) &(server_addr.sin_addr.s_addr)) != 1)
	{
		cerr << "Can not convert little-endian to big-endian" << endl;

		return 1;
	}

	// Start from first param
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] != '-') continue;

		switch (argv[i][1])
		{
		case 'a':
			if (inet_pton(AF_INET, argv[i + 1], (void*) &(server_addr.sin_addr.s_addr)) != 1)
			{
				cerr << "Can not convert little-endian to big-endian" << endl;

				return 1;
			}

			break;

		case 'p':
			server_addr.sin_port = htons((u_short) stoi(argv[i + 1]));
			break;
		}
	}

	char buffer[MESSAGE_SIZE];
	int ret;

	string user_choose;


	if (connect(client, (sockaddr *) &server_addr, sizeof(server_addr)))
	{
		cerr << "Error! Can not connect to server! Error: " << WSAGetLastError() << endl;

		return 1;
	}

	cout << "Connected!" << endl;

	for (;;)
	{
		cout << "Choose 1:\n"
			<< "USER: enter username\n"
			<< "PASS: enter password\n"
			<< "LOUT: logout"
			<< endl;
		cin >> user_choose;

		if (user_choose == MESSAGE_USER)
		{
			string username;
			cin >> username;

			message client_message;

			// Convert string to char*
			const char* type = user_choose.c_str();
			strcpy(client_message.type, type);

			const char* payload = username.c_str();
			strcpy(client_message.payload, payload);

			// Convert from struct message to char*
			memcpy(buffer, &client_message, MESSAGE_SIZE);

			ret = send(client, buffer, MESSAGE_SIZE, 0);

			if (ret == SOCKET_ERROR)
			{
				cerr << "Error: " << WSAGetLastError() << endl;
			}
			else if (ret > 0)
			{
				ret = recv(client, buffer, MESSAGE_SIZE, 0);

				if (ret == SOCKET_ERROR)
				{
					cerr << "Error: " << WSAGetLastError() << endl;
				}
				else if (ret > 0)
				{
					buffer[ret] = 0;

					if (strcmp(buffer, ASK_PASSWORD) == 0)
					{
						cout << "User found! Now you can enter password!" << endl;
					}
					else if (strcmp(buffer, USER_NOT_FOUND) == 0)
					{
						cout << USER_NOT_FOUND << endl;
					}
					else
					{
						cout << MY_ERROR << endl;
					}
				}
			}
			else
			{
				cerr << "Random error!" << endl;
			}
		}
		else if (user_choose == MESSAGE_PASS)
		{
			string password;
			cin >> password;

			message client_message;

			// Convert from string to char*
			const char* type = user_choose.c_str();
			strcpy(client_message.type, type);

			const char* payload = password.c_str();
			strcpy(client_message.payload, payload);

			// Convert from struct message to char*
			memcpy(buffer, &client_message, MESSAGE_SIZE);

			ret = send(client, buffer, MESSAGE_SIZE, 0);

			if (ret == SOCKET_ERROR)
			{
				cerr << "Error: " << WSAGetLastError() << endl;
			}
			else if (ret > 0)
			{
				ret = recv(client, buffer, MESSAGE_SIZE, 0);

				if (ret == SOCKET_ERROR)
				{
					cerr << "Error: " << WSAGetLastError() << endl;
				}
				else if (ret > 0)
				{
					buffer[ret] = 0;

					if (strcmp(buffer, SUCCESS) == 0)
					{
						cout << "Login successed!" << endl;
					}
					else if (strcmp(buffer, WRONG_PASSWORD) == 0)
					{
						cout << WRONG_PASSWORD << endl;
					}
					else
					{
						cout << MY_ERROR << endl;
					}
				}
			}
		}
		else if (user_choose == MESSAGE_LOGOUT)
		{
			string username;
			cin >> username;

			message client_message;

			// Convert from string to char*
			const char* type = user_choose.c_str();
			strcpy(client_message.type, type);

			const char* payload = username.c_str();
			strcpy(client_message.payload, payload);

			// Convert struct message to char*
			memcpy(buffer, &client_message, MESSAGE_SIZE);

			ret = send(client, buffer, MESSAGE_SIZE, 0);

			if (ret == SOCKET_ERROR)
			{
				cerr << "Error: " << WSAGetLastError() << endl;
			}
			else if (ret > 0)
			{
				ret = recv(client, buffer, MESSAGE_SIZE, 0);

				if (ret == SOCKET_ERROR)
				{
					cerr << "Error: " << WSAGetLastError() << endl;
				}
				else if (ret > 0)
				{
					buffer[ret] = 0;

					if (strcmp(buffer, SUCCESS) == 0)
					{
						cout << "Logouted!" << endl;
					}
					else
					{
						cerr << "Error!" << endl;
					}

					break;
				}
			}
		}
		else
		{
			cout << "Invaild option!" << endl;
		}
	}

	shutdown(client, SD_SEND);
	closesocket(client);

	WSACleanup();

	return 0;
}