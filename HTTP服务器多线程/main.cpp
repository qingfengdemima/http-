#include <iostream>
#include <string>
#include <WinSock2.h>
#include <pthread.h>
#include <fstream>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"x64/pthreadVC2.lib")

#define BUFF_SIZE 1024

SOCKET CreateSocket(int _Port);
int WaitConnect(SOCKET _ListenSocket);
void* HttpThread(void* arg);
std::string FromHttpPackageGetUrl(std::string _HttpPackage);
std::string GetFileType(std::string _filename);

int main()
{
    // ��������Ȩ��
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        int errorCode = WSAGetLastError();
        std::cout << "WSAStartup error: " << errorCode << std::endl;
        return -1;
    }

    SOCKET listen_sock = CreateSocket(9090);
    return WaitConnect(listen_sock);
}

SOCKET CreateSocket(int _Port)
{
    // ���磨TCP�������ڼ�����socket��λ ipconfig
    // AF_INET : IPV4
    // int type, ��ʽ  ֡ʽ
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET)
    {
        int errorCode = WSAGetLastError();
        std::cout << "Socket creation error: " << errorCode << std::endl;
        exit(-1);
    }

    struct sockaddr_in local = { 0 };
    local.sin_family = AF_INET;
    local.sin_port = htons(_Port);
    // ����˭��������
    //�κ���
    local.sin_addr.S_un.S_addr = INADDR_ANY;

    // ��
    if (bind(listen_sock, (struct sockaddr*)&local, sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
        int errorCode = WSAGetLastError();
        std::cout << "Bind error: " << errorCode << std::endl;
        exit(-1);
    }
    // ����
    if (listen(listen_sock, 10) == SOCKET_ERROR)
    {
        int errorCode = WSAGetLastError();
        std::cout << "Listen error: " << errorCode << std::endl;
        exit(-1);
    }
    std::cout << "bind and listen success,wait client connect ... " << std::endl;

    return listen_sock;
}

int WaitConnect(SOCKET _ListenSocket)
{
    struct sockaddr_in ClientAddr;
    int socklen = sizeof(struct sockaddr_in);

    pthread_t thread;
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);// �����̷߳���

    while (1)
    {
        SOCKET client_sock = accept(_ListenSocket, (struct sockaddr*)&ClientAddr, &socklen);
        if (client_sock == INVALID_SOCKET)
        {
            int errorCode = WSAGetLastError();
            std::cout << "Accept error: " << errorCode << std::endl;
            continue;
        }

        // ���ӳɹ�
        std::cout << inet_ntoa(ClientAddr.sin_addr) << " " << ntohs(ClientAddr.sin_port) << std::endl;

        // ���߳�
        SOCKET* temp = new SOCKET;
        *temp = client_sock;
        if (pthread_create(&thread, &thread_attr, HttpThread, temp) != 0)
        {
            std::cout << "Thread creation failed" << std::endl;
            closesocket(client_sock);
            delete temp;
        }
    }
}

void* HttpThread(void* arg)
{
    if (arg == nullptr)
        return nullptr;

    SOCKET client_sock = *((SOCKET*)arg);
    delete (SOCKET*)arg;

    // ��������
    char buff[BUFF_SIZE] = { 0 };
    int ret = recv(client_sock, buff, BUFF_SIZE, 0);
    if (ret > 0)
    {
        // ��������
        std::string url = FromHttpPackageGetUrl(buff);
        std::string filecontent = "";
        std::string filetype = "";
        // ��ȡ�ļ�
        if (url == "/")
        {
            // ��ȡindex.html�ļ�����
            std::ifstream file("index.html", std::ios::binary);
            if (file.is_open())
            {
                // ��ȡ�ļ���С
                file.seekg(0, std::ios::end);
                filecontent.resize(file.tellg());
                file.seekg(0, std::ios::beg);
                // ��ȡ�ļ�
                file.read(&filecontent[0], filecontent.size());
                file.close();

                filetype = GetFileType("index.html");
            }
        }
        else
        {
            url = url.substr(1);
            // ��ȡ�ļ�����
            std::ifstream file(url, std::ios::binary);
            if (file.is_open())
            {
                // ��ȡ�ļ���С
                file.seekg(0, std::ios::end);
                filecontent.resize(file.tellg());
                file.seekg(0, std::ios::beg);
                // ��ȡ�ļ�
                file.read(&filecontent[0], filecontent.size());
                file.close();

                filetype = GetFileType(url);
            }
        }

        // �齨HTTP��
        memset(buff, 0, BUFF_SIZE);
        sprintf(buff, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nTransfer-Encoding: chunked\r\nConnection: Keep-Alive\r\nAccept-Ranges:bytes\r\nContent-Length:%d\r\n\r\n", filetype.c_str(), (int)filecontent.size());
        // ����
        send(client_sock, buff, (int)strlen(buff), 0);
        send(client_sock, filecontent.c_str(), (int)filecontent.size(), 0);

    }

    closesocket(client_sock);
    return NULL;
}

std::string FromHttpPackageGetUrl(std::string _HttpPackage)
{
    std::string url = "";
    int start = _HttpPackage.find('/');
    int end = _HttpPackage.find(' ', start);
    url = _HttpPackage.substr(start, end - start);
    return url;
}
//�õ��ļ�����
//���Ͳ���ȫ���������Ϳɽ��з���
std::string GetFileType(std::string _filename)
{
    size_t start = _filename.rfind('.');
    std::string Suffix = _filename.substr(start + 1);

    if (Suffix == "bmp")
        return "image/bmp";
    else if (Suffix == "gif")
        return "image/gif";
    else if (Suffix == "ico")
        return "image/x-icon";
    else if (Suffix == "jpg")
        return "image/jpeg";
    else if (Suffix == "avi")
        return "video/avi";
    else if (Suffix == "css")
        return "text/css";
    else if (Suffix == "dll" || Suffix == "exe")
        return "application/x-msdownload";
    else if (Suffix == "dtd")
        return "text/xml";
    else if (Suffix == "mp3")
        return "audio/mp3";
    else if (Suffix == "mpg")
        return "video/mpg";
    else if (Suffix == "png")
        return "image/png";
    else if (Suffix == "xls")
        return "application/vnd.ms-excel";
    else if (Suffix == "doc")
        return "application/msword";
    else if (Suffix == "mp4")
        return "video/mpeg4";
    else if (Suffix == "ppt")
        return "application/x-ppt";
    else if (Suffix == "wma")
        return "audio/x-ms-wma";
    else if (Suffix == "wmv")
        return "video/x-ms-wmv";

    return "text/html";//��û��ƥ���Ϸ������
}