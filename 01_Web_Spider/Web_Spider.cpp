
/*******************************************C++网络爬虫*******************************************************/
/****************************************能够实现对网页内容的抓取********************************************/

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


void startupWSA();                              //初始化套接字						 
pair<string, string> binaryString(const string &str, const string &dilme); //URL 是 host + get.因此需要一个 binaryString 把它切开
SOCKET connect(const string &hostName);		   //创建套接字并请求连接
bool sendRequest(SOCKET sock, const string &host, const string &get);      //向目标URL的套接字发送消息
string recvRequest(SOCKET sock);               //从目标URL的套接字接收消息
void extUrl(const string &buffer, queue<string> &urlQueue);			       //用正则表达式将从网页抓取的URL存放到队列中
void cleanupWSA();                             //终止Winsock 2 DLL (Ws2_32.dll) 的使用
void Go(const string &url, int count);//对以上函数的调用

void main()
{
	startupWSA();
	Go("www.hao123.com", 200);
	cleanupWSA();
}



//初始化套接字
void startupWSA()                                           
{
	WSADATA wsadata;  
	WSAStartup(MAKEWORD(2, 0), &wsadata);
}

//WSACleanup()是一个计算机函数，功能是终止Winsock 2 DLL (Ws2_32.dll) 的使用
inline void cleanupWSA()
{
	WSACleanup();
}

//URL 是 host + get.因此需要一个 binaryString 把它切开
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

//解析URL，返回IP
inline string getIpByHostName(const string &hostName)       
{
	hostent* phost = gethostbyname(hostName.c_str());
	return phost ? inet_ntoa(*(in_addr *)phost->h_addr_list[0]) : "";
}

//创建套接字并请求连接
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

//向目标URL的套接字发送消息
inline bool sendRequest(SOCKET sock, const string &host, const string &get)
{
	string http
		= "GET " + get + " HTTP/1.1\r\n"
		+ "HOST: " + host + "\r\n"
		+ "Connection: close\r\n\r\n";
	return http.size() == send(sock, &http[0], http.size(), 0);
}

//从目标URL的套接字接收消息
inline string recvRequest(SOCKET sock)
{
	static timeval wait = { 2, 0 };  //设置一个时间周期
	static string buffer = string(2048 * 100, '\0');
	int len = 0, reclen = 0;
	do {
		fd_set fd = { 0 };
		FD_SET(sock, &fd);
		/*select()机制中提供一fd_set的数据结构，实际上是一long类型的数组，
		每一个数组元素都能与一打开的文件句柄（不管是socket句柄，还是其他文件或命名管道或设备句柄）建立联系，
		建立联系的工作由程序员完成，当调用select()时，由内核根据IO状态修改fd_set的内容，
		由此来通知执行了select()的进程哪一socket或文件发生了可读或可写事件。*/
		reclen = 0;
		if (select(0, &fd, nullptr, nullptr, &wait) > 0)//返回值：准备就绪的描述符数，若超时则返回0，若出错则返回-1。
		{
			reclen = recv(sock, &buffer[0] + len, 2048 * 100 - len, 0);//返回接收长度
			if (reclen > 0)
				len += reclen;
		}
		FD_ZERO(&fd);//初始化套接字集合（其实就是清空套接字集合）
	} while (reclen > 0);

	return len > 11
		? buffer[9] == '2' && buffer[10] == '0' && buffer[11] == '0'
		? buffer.substr(0, len)
		: ""
		: "";
}

//正则表达式，将从网页抓取的URL存放到队列中
inline void extUrl(const string &buffer, queue<string> &urlQueue)
{
	if (buffer.empty())
	{
		return;
	}
	smatch result;
	string::const_iterator curIter = buffer.begin();
	string::const_iterator endIter = buffer.end();       //在regex_search函数中，会将找到的第一个匹配结果保存到一个smatch类中。
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
	queue<string> urls;//队列,先进先出
	urls.push(url);
	for (int i = 0; i != count; ++i)
	{
		if (!urls.empty())
		{
			string &url = urls.front();   //取出队列第一个
			pair<string, string> pair = binaryString(url, "/");
			SOCKET sock = connect(pair.first);
			if (sock && sendRequest(sock, pair.first, "/" + pair.second))
			{                   
				string buffer = move(recvRequest(sock)); //std::move函数可以以非常简单的方式将左值引用转换为右值引用。
				extUrl(buffer, urls);
			}
			closesocket(sock);       //将从网页抓取的URL存放到队列中
			cout << url << ": 此count=> " << urls.size() << endl;//输出队列中URL数目
			urls.pop();                //移除队列第一个

		}
	}
}

