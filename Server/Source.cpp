#include<WinSock2.h>
#include<WS2tcpip.h>
#include <process.h>
#include<iostream>
#include<string>
#include<cstdbool>
#include<fstream>
#include<vector>

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

typedef struct
{
	boolean is_authenticated = false;
	boolean is_waiting_password = false;
	string username;
	string password;
} user;

vector<user> users_list;

// <summary>Verify username</summary>
//	<returns>
//		0: user found
//		1: user not found
//		-1: error
//	</returns>
int verify_username(string username, vector<user> &users_list)
{
	for (user &user : users_list)
	{
		if (user.username == username)
		{
			if (user.is_authenticated)
			{
				return -1;
			}
			else
			{
				user.is_waiting_password = true;
				return 0;
			}
		}
	}
	return 1;
}

// <summary>Verify password</summary>
//	<returns>
//		1: wrong user
//		0: logined
//		-1: error
//	</returns>
int verify_password(string password, vector<user> &users_list)
{
	for (user &user : users_list)
	{
		if (user.password == password)
		{
			if (user.is_waiting_password)
			{
				user.is_authenticated = true;
				user.is_waiting_password = false;
				return 0;
			}
			else
			{
				return 1;
			}
		}
	}
	return -1;
}

// <summary>Logout</summary>
//	<returns>
//		0: successed
//		1: error
//	</returns>
int logout(string username, vector<user> &users_list)
{
	for (user &user : users_list)
	{
		if (user.username == username)
		{
			user.is_authenticated = false;
			user.is_waiting_password = false;
			return 0;
		}
	}
	return 1;
}

unsigned __stdcall handler(void* param)
{
	SOCKET connect_socket = (SOCKET) param;
	char buffer[MESSAGE_SIZE];
	int ret;

	for (;;)
	{
		ret = recv(connect_socket, buffer, MESSAGE_SIZE, 0);

		if (ret == SOCKET_ERROR)
		{
			cerr << "Error: " << WSAGetLastError() << endl;
			break;
		}
		else if (ret > 0)
		{
			message client_message;

			// Convert from char* to message (buffer contains 2 array end with null)
			strncpy(client_message.type, buffer, 5);
			strncpy(client_message.payload, buffer + 5, ret - 5);

			if (strcmp(client_message.type, MESSAGE_USER) == 0)
			{
				string username(client_message.payload);

				int result = verify_username(username, users_list);
				if (result == 0)
				{
					if (send(connect_socket, ASK_PASSWORD, sizeof(ASK_PASSWORD), 0) == SOCKET_ERROR)
					{
						cerr << "Error: " << WSAGetLastError() << endl;
						break;
					}
				}
				else if (result == 1)
				{
					if (send(connect_socket, USER_NOT_FOUND, sizeof(USER_NOT_FOUND), 0) == SOCKET_ERROR)
					{
						cerr << "Error: " << WSAGetLastError() << endl;
						break;
					}
				}
				else
				{
					if (send(connect_socket, MY_ERROR, sizeof(MY_ERROR), 0) == SOCKET_ERROR)
					{
						cerr << "Error: " << WSAGetLastError() << endl;
						break;
					}
				}
			}
			else if (strcmp(client_message.type, MESSAGE_PASS) == 0)
			{
				string password(client_message.payload);

				int result = verify_password(password, users_list);
				if (result == 0)
				{
					if (send(connect_socket, SUCCESS, sizeof(SUCCESS), 0) == SOCKET_ERROR)
					{
						cerr << "Error: " << WSAGetLastError() << endl;
						break;
					}
				}
				else
				{
					if (send(connect_socket, WRONG_PASSWORD, sizeof(WRONG_PASSWORD), 0) == SOCKET_ERROR)
					{
						cerr << "Error: " << WSAGetLastError() << endl;
						break;
					}
				}
			}
			else if (strcmp(client_message.type, MESSAGE_LOGOUT) == 0)
			{
				string username(client_message.payload);

				if (logout(username, users_list) == 0)
				{
					if (send(connect_socket, SUCCESS, sizeof(SUCCESS), 0) == SOCKET_ERROR)
					{
						cerr << "Error: " << WSAGetLastError() << endl;
					}

					break;
				}
				else
				{
					if (send(connect_socket, USER_NOT_FOUND, sizeof(USER_NOT_FOUND), 0) == SOCKET_ERROR)
					{
						cerr << "Error: " << WSAGetLastError() << endl;
						break;
					}
				}
			}
		}
		else
		{
			if (send(connect_socket, MY_ERROR, sizeof(MY_ERROR), 0) == SOCKET_ERROR)
			{
				cerr << "Error: " << WSAGetLastError() << endl;
				break;
			}
		}
	}

	shutdown(connect_socket, SD_SEND);
	closesocket(connect_socket);

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		cerr << "Missing arguments!" << endl;

		return 1;
	}

	WSADATA	wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
	{
		cerr << "Version is not supported!" << endl;
	}

	SOCKET listen_socket;
	listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((u_short) SERVER_PORT);

	if (inet_pton(AF_INET, SERVER_ADDRESS, (void*) &(server_addr.sin_addr.s_addr)) != 1)
	{
		cerr << "Can not convert little-endian to big-endian" << endl;

		return 1;
	}

	// Start from first argument
	for (int i = 1; i <= argc && argv[i][0] == '-'; ++i)
	{
		if (argv[i][1] == 'p')
		{
			server_addr.sin_port = htons((u_short) stoi(argv[i + 1]));

			break;
		}
	}

	if (bind(listen_socket, (sockaddr *) &server_addr, sizeof(server_addr)))
	{
		cerr << "Can not bind to this address!" << endl;

		return 1;
	}

	if (listen(listen_socket, 10))
	{
		cerr << "Can not listen!" << endl;

		return 1;
	}

	cout << "Server started!" << endl;

	ifstream file("Users.txt");
	if (file.is_open())
	{
		// Read users' data
		while (!file.eof())
		{
			user user;

			string username, password;
			file >> username >> password;

			user.username = username;
			user.password = password;

			users_list.push_back(user);
		}
	}
	else
	{
		cerr << "Can not open file!" << endl;
		return 1;
	}

	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);

	for (;;)
	{
		SOCKET connect_socket = accept(listen_socket, (sockaddr *) &client_addr, &client_addr_len);

		_beginthreadex(0, 0, handler, (void *) connect_socket, 0, 0);
	}

	WSACleanup();

	return 0;
}
