// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"

HANDLE g_eventHandle;

void message_received(
    _In_ HCWebsocketHandle websocket,
    _In_z_ const char* incomingBodyString
    )
{
    printf_s("Received websocket message: %s\n", incomingBodyString);
    SetEvent(g_eventHandle);
}

void websocket_closed(
    _In_ HCWebsocketHandle websocket,
    _In_ HCWebSocketCloseStatus closeStatus
    )
{
    printf_s("Websocket closed!\n");
    SetEvent(g_eventHandle);
}

int main()
{
    g_eventHandle = CreateEvent(nullptr, false, false, nullptr);

    HCInitialize(nullptr);
    HCSettingsSetTraceLevel(HCTraceLevel::Verbose);

    XTaskQueueHandle queue;
    XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::ThreadPool,
        &queue);

    std::string url = "wss://echo.websocket.org";

    HCWebsocketHandle websocket;
    HRESULT hr = HCWebSocketCreate(&websocket);
    hr = HCWebSocketSetFunctions(message_received, websocket_closed);

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = queue;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        WebSocketCompletionResult result = {};
        HCGetWebSocketConnectResult(asyncBlock, &result);

        printf_s("HCWebSocketConnect complete: %d, %d\n", result.errorCode, result.platformErrorCode);
        SetEvent(g_eventHandle);
        delete asyncBlock;
    };

    printf_s("Calling HCWebSocketConnect...\n");
    hr = HCWebSocketConnectAsync(url.data(), "", websocket, asyncBlock);
    WaitForSingleObject(g_eventHandle, INFINITE);

    asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = queue;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        WebSocketCompletionResult result = {};
        HCGetWebSocketSendMessageResult(asyncBlock, &result);

        printf_s("HCWebSocketSendMessage complete: %d, %d\n", result.errorCode, result.platformErrorCode);
        SetEvent(g_eventHandle);
        delete asyncBlock;
    };

    std::string requestString = "This message should be echoed!";
    printf_s("Calling HCWebSocketSend with message \"%s\" and waiting for response...\n", requestString.data());
    hr = HCWebSocketSendMessageAsync(websocket, requestString.data(), asyncBlock);
    
    // Wait for send to complete successfully and then wait again for response to be received.
    WaitForSingleObject(g_eventHandle, INFINITE);
    WaitForSingleObject(g_eventHandle, INFINITE);

    printf_s("Calling HCWebSocketDisconnect...\n");
    HCWebSocketDisconnect(websocket);
    WaitForSingleObject(g_eventHandle, INFINITE);

    HCWebSocketCloseHandle(websocket);
    CloseHandle(g_eventHandle);
    return 0;
}

