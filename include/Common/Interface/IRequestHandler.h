// 작성자: bumpsgoodman

#ifndef __I_REQUEST_HANDLER_H
#define __I_REQUEST_HANDLER_H

#define INTERFACE struct

typedef INTERFACE IRequestHandler IRequestHandler;
INTERFACE IRequestHandler
{
    bool (*Init)(IRequestHandler* pThis); 
};

#endif // __I_REQUEST_HANDLER_H