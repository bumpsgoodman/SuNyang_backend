// 작성자: bumpsgoodman

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../Common/Assert.h"
#include "../Common/ErrorHandler.h"
#include "../Common/INIParser.h"
#include "../Common/PrimitiveType.h"

typedef struct SERVER_INFO
{
    uint16_t HttpPort;
    uint16_t HttpsPort;

    char* pCertFilePath;
    char* pPrivateKeyFilePath;
} SERVER_INFO;

bool InitServerInfo(const char* pFilename, SERVER_INFO* pOutInfo);
bool InitSSL(const char* pCertFilePath, const char* pPrivateKeyFilePath, SSL_CTX** ppOutCTX);

bool CreateSocket(const uint16_t port, int* pOutSocket);
bool BindSocket(const int sock, const uint16_t port);

void add_cors_headers(int newsockfd);

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Error: Unable to read server information file.\n");
        goto lb_return;
    }

    // 서버 정보 초기화
    SERVER_INFO serverInfo;
    if (!InitServerInfo(argv[1], &serverInfo))
    {
        goto lb_return;
    }

    // SSL 초기화
    SSL_CTX* pCTX;
    if (!InitSSL(serverInfo.pCertFilePath, serverInfo.pPrivateKeyFilePath, &pCTX))
    {
        goto lb_return;
    }
    
    // 소켓 생성 및 바인딩
    int httpSock;
    int httpsSock;
    CreateSocket(serverInfo.HttpPort, &httpSock);
    CreateSocket(serverInfo.HttpsPort, &httpsSock);

    // 소켓 바인딩
    BindSocket(httpSock, serverInfo.HttpPort);
    BindSocket(httpsSock, serverInfo.HttpsPort);

    listen(httpSock, 5);
    listen(httpsSock, 5);

    // 클라이언트 소켓 생성
    struct sockaddr_in clientAddr = { 0, };
    socklen_t clientLen = sizeof(clientAddr);
    int clientSock;
    SSL* pSSL = NULL;
    while (true)
    {
        clientSock = accept(httpsSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0)
        {
            fprintf(stderr, "Error: Failed to create accept socket.\n");
            goto lb_return;
        }

        // SSL 생성
        SSL*pSSL = SSL_new(pCTX);
        if (!pSSL)
        {
            ERR_print_errors_fp(stderr);
            goto lb_return;
        }

        // SSL 설정
        SSL_set_fd(pSSL, clientSock);

        // SSL 연결
        int ret = SSL_accept(pSSL);
        if (ret != 1)
        {
            ERR_print_errors_fp(stderr);
            goto lb_return;
        }

        char buffer[1024] = { 0, };
        int n = SSL_read(pSSL, buffer, 1023);
        if (n < 0)
        {
            fprintf(stderr, "Error: Failed to read from socket\n");
            goto lb_return;
        }

        printf("%s\n",buffer);

        char response[] = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 19\r\n"
            "\r\n"
            "Hello, HTTPS World!\r\n";
        SSL_write(pSSL, response, strlen(response));

        SSL_shutdown(pSSL);
        SSL_free(pSSL);
        close(clientSock);
    }

lb_return:
    if (clientSock >= 0)
    {
        close(clientSock);
    }

    if (httpSock >= 0)
    {
        close(httpSock);
    }

    if (httpsSock >= 0)
    {
        close(httpsSock);
    }

    if (pSSL != NULL)
    {
        SSL_shutdown(pSSL);
        SSL_free(pSSL);
    }

    if (pCTX != NULL)
    {
        SSL_CTX_free(pCTX);
    }

    return 0;
}

bool InitSSL(const char* pCertFilePath, const char* pPrivateKeyFilePath, SSL_CTX** ppOutCTX)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    *ppOutCTX = NULL;

    SSL_CTX* pCTX = SSL_CTX_new(SSLv23_server_method());
    if (pCTX == NULL)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_certificate_file(pCTX, pCertFilePath, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(pCTX, pPrivateKeyFilePath, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    *ppOutCTX = pCTX;
    return true;
}

bool InitServerInfo(const char* pFilename, SERVER_INFO* pOutInfo)
{
    ASSERT(pFilename != NULL, "pFilename is NULL");
    ASSERT(pOutInfo != NULL, "pOutInfo is NULL");

    // .ini 파일인지 검사
    const size_t length = strlen(pFilename);
    if (length < 4
        || pFilename[length - 1] != 'i'
        || pFilename[length - 2] != 'n'
        || pFilename[length - 3] != 'i'
        || pFilename[length - 4] != '.')
    {
        fprintf(stderr, "Error: Unable to read server information file.\n");
        return false;
    }

    INI_PARSER parser;
    INIParser_Init(&parser);

    INIParser_Parse(&parser, pFilename);
    INIParser_GetValueShort(&parser, "Server", "httpPort", (short*)&pOutInfo->HttpPort);
    INIParser_GetValueShort(&parser, "Server", "httpsPort", (short*)&pOutInfo->HttpsPort);
    INIParser_GetValueString(&parser, "Server", "certPath", &pOutInfo->pCertFilePath);
    INIParser_GetValueString(&parser, "Server", "privateKeyPath", &pOutInfo->pPrivateKeyFilePath);

    INIParser_Release(&parser);

    return true;
}

bool CreateSocket(const uint16_t port, int* pOutSocket)
{
    ASSERT(pOutSocket != NULL, "pOutSocket is NULL");

    // 소켓 생성
    const int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error: Failed to create socket.\n");
        return false;
    }

    *pOutSocket = sockfd;
    return true;
}

bool BindSocket(const int sock, const uint16_t port)
{
    ASSERT(sock >= 0, "Invalid soket.");

    // 소켓 바인딩
    struct sockaddr_in serverAddr = { 0, };
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        fprintf(stderr, "Error: Failed to bind socket.\n");
        return false;
    }

    return true;
}

// CORS 헤더 설정 함수 추가
void add_cors_headers(int newsockfd) {
    const char *headers = 
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "\r\n";
    write(newsockfd, headers, strlen(headers));
}