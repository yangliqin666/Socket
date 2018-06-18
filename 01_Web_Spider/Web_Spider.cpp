
/*******************************************C++��������*******************************************************/
/****************************************�ܹ�ʵ�ֶ���ҳ���ݵ�ץȡ********************************************/

#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <regex>
#include <fstream>
#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")
using namespace std;


void startupWSA();                              //��ʼ���׽���						 
pair<string, string> binaryString(const string &str, const string &dilme); //URL �� host + get.�����Ҫһ�� binaryString �����п�
SOCKET connect(const string &hostName);		   //�����׽��ֲ���������
bool sendRequest(SOCKET sock, const string &host, const string &get);      //��Ŀ��URL���׽��ַ�����Ϣ
string recvRequest(SOCKET sock);               //��Ŀ��URL���׽��ֽ�����Ϣ
void extUrl(const string &buffer, queue<string> &urlQueue);			       //��������ʽ������ҳץȡ��URL��ŵ�������
void cleanupWSA();                             //��ֹWinsock 2 DLL (Ws2_32.dll) ��ʹ��
void Go(const string &url, int count);//�����Ϻ����ĵ���

void main()
{
	startupWSA();
	Go("www.hao123.com", 200);
	cleanupWSA();
}



//��ʼ���׽���
void startupWSA()                                           
{
	WSADATA wsadata;  
	WSAStartup(MAKEWORD(2, 0), &wsadata);
}

//WSACleanup()��һ���������������������ֹWinsock 2 DLL (Ws2_32.dll) ��ʹ��
inline void cleanupWSA()
{
	WSACleanup();
}

//URL �� host + get.�����Ҫһ�� binaryString �����п�
inline pair<string, string> binaryString(const string &str, const string &dilme)
{
	pair<string, string> result(str, "");
	int pos = str.find(dilme);
	if (pos != string::npos)
	{
		result.first = str.substr(0, pos);
		result.second = str.substr(pos + dilme.size());
	}
	return result;
}

//����URL������IP
inline string getIpByHostName(const string &hostName)       
{
	hostent* phost = gethostbyname(hostName.c_str());
	return phost ? inet_ntoa(*(in_addr *)phost->h_addr_list[0]) : "";
}

//�����׽��ֲ���������
inline SOCKET connect(const string &hostName)
{
	string ip = getIpByHostName(hostName);
	if (ip.empty())
		return 0;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		return 0;
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	if (connect(sock, (const sockaddr *)&addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
		return 0;
	return sock;
}

//��Ŀ��URL���׽��ַ�����Ϣ
inline bool sendRequest(SOCKET sock, const string &host, const string &get)
{
	string http
		= "GET " + get + " HTTP/1.1\r\n"
		+ "HOST: " + host + "\r\n"
		+ "Connection: close\r\n\r\n";
	return http.size() == send(sock, &http[0], http.size(), 0);
}

//��Ŀ��URL���׽��ֽ�����Ϣ
inline string recvRequest(SOCKET sock)
{
	static timeval wait = { 2, 0 };  //����һ��ʱ������
	static string buffer = string(2048 * 100, '\0');
	int len = 0, reclen = 0;
	do {
		fd_set fd = { 0 };
		FD_SET(sock, &fd);
		/*select()�������ṩһfd_set�����ݽṹ��ʵ������һlong���͵����飬
		ÿһ������Ԫ�ض�����һ�򿪵��ļ������������socket��������������ļ��������ܵ����豸�����������ϵ��
		������ϵ�Ĺ����ɳ���Ա��ɣ�������select()ʱ�����ں˸���IO״̬�޸�fd_set�����ݣ�
		�ɴ���ִ֪ͨ����select()�Ľ�����һsocket���ļ������˿ɶ����д�¼���*/
		reclen = 0;
		if (select(0, &fd, nullptr, nullptr, &wait) > 0)//����ֵ��׼����������������������ʱ�򷵻�0���������򷵻�-1��
		{
			reclen = recv(sock, &buffer[0] + len, 2048 * 100 - len, 0);//���ؽ��ճ���
			if (reclen > 0)
				len += reclen;
		}
		FD_ZERO(&fd);//��ʼ���׽��ּ��ϣ���ʵ��������׽��ּ��ϣ�
	} while (reclen > 0);

	return len > 11
		? buffer[9] == '2' && buffer[10] == '0' && buffer[11] == '0'
		? buffer.substr(0, len)
		: ""
		: "";
}

//������ʽ��������ҳץȡ��URL��ŵ�������
inline void extUrl(const string &buffer, queue<string> &urlQueue)
{
	if (buffer.empty())
	{
		return;
	}
	smatch result;
	string::const_iterator curIter = buffer.begin();
	string::const_iterator endIter = buffer.end();       //��regex_search�����У��Ὣ�ҵ��ĵ�һ��ƥ�������浽һ��smatch���С�
	while (regex_search(curIter, endIter, result, regex("href=\"(https?:)?//\\S+\"")))
	{
		urlQueue.push(regex_replace(
			result[0].str(),
			regex("href=\"(https?:)?//(\\S+)\""),
			"$2"));
		curIter = result[0].second;
	}
}

void Go(const string &url, int count)
{
	queue<string> urls;//����,�Ƚ��ȳ�
	urls.push(url);
	for (int i = 0; i != count; ++i)
	{
		if (!urls.empty())
		{
			string &url = urls.front();   //ȡ�����е�һ��
			pair<string, string> pair = binaryString(url, "/");
			SOCKET sock = connect(pair.first);
			if (sock && sendRequest(sock, pair.first, "/" + pair.second))
			{                   
				string buffer = move(recvRequest(sock)); //std::move���������Էǳ��򵥵ķ�ʽ����ֵ����ת��Ϊ��ֵ���á�
				extUrl(buffer, urls);
			}
			closesocket(sock);       //������ҳץȡ��URL��ŵ�������
			cout << url << ": ��count=> " << urls.size() << endl;//���������URL��Ŀ
			urls.pop();                //�Ƴ����е�һ��

		}
	}
}

