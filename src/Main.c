// 작성자: bumpsgoodman

#include "RequestHandler/HttpMessage.h"
#include "RequestHandler/HttpRedirector.h"
#include "Common/Assert.h"
#include "Common/ErrorHandler.h"
#include "Common/INIParser.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"

#include "Logger/Logger.h"

#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAX_HTTP_MESSAGE_SIZE 8192
#define IPV4_SIZE 16

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

bool CreateSslAndAccept(SSL_CTX* pCTX, const int sock, SSL** ppOutSSL);

void ResponseCorsHeaders(SSL* pSSL);

void Ipv4ToString(const uint32_t ipv4, char* pOutBytes);

int main(int argc, char* argv[])
{   
    Logger_Print(LOG_LEVEL_INFO, "START SUNYANGI SERVER.");

    if (argc < 2)
    {
        Logger_Print(LOG_LEVEL_ERROR, "Unable to read server information file.");
        goto lb_return;
    }

    // 서버 정보 초기화
    SERVER_INFO serverInfo;
    if (!InitServerInfo(argv[1], &serverInfo))
    {
        Logger_Print(LOG_LEVEL_ERROR, "Unable to read server information .ini file.");
        goto lb_return;
    }

    pthread_t httpRedirectorThread;
    pthread_create(&httpRedirectorThread, NULL, RedirectHttp, (void*)&serverInfo.HttpPort);

    // SSL 초기화
    SSL_CTX* pCTX;
    if (!InitSSL(serverInfo.pCertFilePath, serverInfo.pPrivateKeyFilePath, &pCTX))
    {
        Logger_Print(LOG_LEVEL_ERROR, "Failed to initialize SSL");
        goto lb_return;
    }

    Logger_Print(LOG_LEVEL_INFO, "HTTPS - START");

    // https 소켓 생성
    int httpsSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (httpsSock < 0)
    {
        Logger_Print(LOG_LEVEL_ERROR, "Failed to create HTTPS socket");
    }

    int opt = 1;
    if (setsockopt(httpsSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        Logger_Print(LOG_LEVEL_ERROR, "HTTPS - Failed to set socket options");
        goto lb_return;
    }

    // https 소켓 바인딩
    if (!BindSocket(httpsSock, serverInfo.HttpsPort))
    {
        Logger_Print(LOG_LEVEL_ERROR, "Failed to bind HTTPS socket.");
        goto lb_return;
    }
    
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
            Logger_Print(LOG_LEVEL_ERROR, "Failed to create client socket.");
            goto lb_release_request;
        }

        if (!CreateSslAndAccept(pCTX, clientSock, &pSSL))
        {
            Logger_Print(LOG_LEVEL_ERROR, "Failed to create SSL.");
            goto lb_release_request;
        }

        char* pBuffer = (char*)malloc(MAX_HTTP_MESSAGE_SIZE);

        int n = SSL_read(pSSL, pBuffer, MAX_HTTP_MESSAGE_SIZE - 1);
        if (n < 0)
        {
            goto lb_release_request;
        }

        START_LINE startLine;
        if (!ParseStartLine(pBuffer, &startLine))
        {
            Logger_Print(LOG_LEVEL_ERROR, "Failed to parse start line.");
            goto lb_release_request;
        }

        char clientIP[INET_ADDRSTRLEN];
        Ipv4ToString(clientAddr.sin_addr.s_addr, clientIP);
        int clientPort = clientAddr.sin_port;

        Logger_Print(LOG_LEVEL_INFO, "%s  %s  HTTP/%d.%d  from %s:%d", GetHttpMethodString(startLine.Method),
                                                    startLine.pRequestTarget,
                                                    startLine.Version.Major,
                                                    startLine.Version.Minor,
                                                    clientIP, clientPort);

        switch (startLine.Method)
        {
        case HTTP_METHOD_GET:
        {
            if (strstr(startLine.pRequestTarget, "/images") != NULL)
            {
                char filename[4096];
                sprintf(filename, "public%s", startLine.pRequestTarget);

                FILE* pImageFile = fopen(filename, "rb");
                if (pImageFile == NULL)
                {
                    goto lb_release_request;
                }

                // 파일 사이즈 구하기
                fseek(pImageFile, 0, SEEK_END);
                const int imageSize = ftell(pImageFile);
                fseek(pImageFile, 0, SEEK_SET);

                char header[128];
                sprintf(header, "HTTP/1.1 200 OK\r\n"
                                "Content-Type: image/png\r\n"
                                "Content-Length: %d\r\n"
                                "\r\n", imageSize);
                Logger_Print(LOG_LEVEL_INFO, header);
                SSL_write(pSSL, header, strlen(header));

                char buffer[4096];
                size_t n;
                while ((n = fread(buffer, 1, 4096, pImageFile)) != 0)
                {
                    SSL_write(pSSL, buffer, n);
                }

                fclose(pImageFile);
            }
            else
            {
                FILE* pFile = fopen("public/index.html", "rb");
                if (pFile == NULL)
                {
                    goto lb_release_request;
                }

                // 파일 사이즈 구하기
                fseek(pFile, 0, SEEK_END);
                const int size = ftell(pFile);
                fseek(pFile, 0, SEEK_SET);

                char header[128];
                sprintf(header, "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html; charset=utf-8\r\n"
                                "Content-Length: %d\r\n"
                                "\r\n", size);
                SSL_write(pSSL, header, strlen(header));

                char buffer[4096];
                size_t n;
                while ((n = fread(buffer, 1, 4096, pFile)) != 0)
                {
                    SSL_write(pSSL, buffer, n);
                }

                fclose(pFile);
            }
            
            break;
        }
        case HTTP_METHOD_OPTIONS:
            ResponseCorsHeaders(pSSL);
            break;
        default:
            break;
        }

    lb_release_request:
        SAFE_FREE(pBuffer);

        if (pSSL != NULL)
        {
            SSL_shutdown(pSSL);
            SSL_free(pSSL);
        }

        if (clientSock >= 0)
        {
            close(clientSock);
        }
    }

lb_return:
    if (clientSock >= 0)
    {
        close(clientSock);
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

    free(serverInfo.pCertFilePath);
    free(serverInfo.pPrivateKeyFilePath);

    Logger_Print(LOG_LEVEL_INFO, "End server");

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
        || pFilename[length - 1] != 'i' && pFilename[length - 1] != 'I'
        || pFilename[length - 2] != 'n' && pFilename[length - 2] != 'N'
        || pFilename[length - 3] != 'i' && pFilename[length - 3] != 'I'
        || pFilename[length - 4] != '.')
    {
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
        return false;
    }

    return true;
}

bool CreateSslAndAccept(SSL_CTX* pCTX, const int sock, SSL** ppOutSSL)
{
    ASSERT(pCTX >= 0, "pCTX is NULL");
    ASSERT(ppOutSSL >= 0, "ppOutSSL is NULL");

    // SSL 생성
    SSL* pSSL = SSL_new(pCTX);
    if (!pSSL)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    // SSL 설정
    SSL_set_fd(pSSL, sock);

    // SSL 연결
    int ret = SSL_accept(pSSL);
    if (ret != 1)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    *ppOutSSL = pSSL;
    return true;
}

// CORS 헤더 설정 함수 추가
void ResponseCorsHeaders(SSL* pSSL)
{
    const char *headers = 
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "\r\n";
    SSL_write(pSSL, headers, strlen(headers));
}

void Ipv4ToString(const uint32_t ipv4, char* pOutBytes)
{
    ASSERT(pOutBytes != NULL, "pOutBytes is NULL");

    const uint8_t a = ipv4 & 0xff;
    const uint8_t b = (ipv4 & 0xff00) >> 8;
    const uint8_t c = (ipv4 & 0xff0000) >> 16;
    const uint8_t d = (ipv4 & 0xff000000) >> 24;

    sprintf(pOutBytes, "%d.%d.%d.%d", a, b, c, d);
}