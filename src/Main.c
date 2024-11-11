// 작성자: bumpsgoodman

#include "ServerInfo.h"
#include "HttpRedirector.h"
#include "Common/Assert.h"
#include "Common/PrimitiveType.h"
#include "Common/ErrorCode/ErrorCode.h"
#include "Common/Logger/Logger.h"

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        Logger_Print(LOG_LEVEL_ERROR, "Unable to read the server info file.");
        goto lb_return;
    }

    bool bResult = true;
    SERVER_INFO serverInfo;
    HTTP_REDIRECTOR httpRedirector;

    // 초기화
    {
        Logger_Print(LOG_LEVEL_INFO, "INITIALIZE SUNYANGI SERVER.");

        bResult = ServerInfo_Init(&serverInfo, argv[1]);
        if (!bResult)
        {
            goto lb_return;
        }

        bResult = HttpRedirector_Init(&httpRedirector, serverInfo.HttpPort);
        if (!bResult)
        {
            goto lb_return;
        }
    }

    // 실행
    {
        Logger_Print(LOG_LEVEL_INFO, "START SUNYANGI SERVER.");

        HttpRedirector_Start(&httpRedirector);
    }

    while (true) ;

lb_return:
    // 해제
    {
        ServerInfo_Release(&serverInfo);
        HttpRedirector_Shutdown(&httpRedirector);
    }

    Logger_Print(LOG_LEVEL_INFO, "END SUNYANGI SERVER.");

    return 0;
}