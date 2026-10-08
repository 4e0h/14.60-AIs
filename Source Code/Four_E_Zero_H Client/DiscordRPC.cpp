#include "DiscordRPC.h"
#include "pch.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char* DISCORD_CLIENT_ID = "PUT_YOUR_CLIENT_ID_HERE";

static const char* RPC_DETAILS = "Playing 14.60 with player ais - ";
static const char* RPC_STATE = ".gg/ZVTBWDjvFv";
static const char* RPC_LARGE_IMAGE = "jordan";
static const char* RPC_LARGE_TEXT = "14.60 AIs";
static const char* RPC_BUTTON_LABEL = "Join Discord";
static const char* RPC_BUTTON_URL = "https://discord.gg/ZVTBWDjvFv";

static const DWORD RPC_REFRESH_MS = 1000;

static volatile LONG g_started = 0;
static volatile LONG g_stop = 0;
static HANDLE g_thread = NULL;
static HANDLE g_pipe = INVALID_HANDLE_VALUE;
static __int64 g_start_ts = 0;

enum {
    OP_HANDSHAKE = 0,
    OP_FRAME = 1,
    OP_CLOSE = 2,
    OP_PING = 3,
    OP_PONG = 4
};

static void ClosePipe(void) {
    if (g_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(g_pipe);
        g_pipe = INVALID_HANDLE_VALUE;
    }
}

static BOOL PipeWrite(const void* data, DWORD len) {
    const char* p = (const char*)data;
    DWORD left = len;
    while (left) {
        DWORD wrote = 0;
        if (!WriteFile(g_pipe, p, left, &wrote, NULL) || wrote == 0)
            return FALSE;
        p += wrote;
        left -= wrote;
    }
    return TRUE;
}

static BOOL PipeRead(void* data, DWORD len) {
    char* p = (char*)data;
    DWORD left = len;
    while (left) {
        DWORD got = 0;
        if (!ReadFile(g_pipe, p, left, &got, NULL) || got == 0)
            return FALSE;
        p += got;
        left -= got;
    }
    return TRUE;
}

static BOOL SendFrame(DWORD op, const char* json) {
    DWORD len = (DWORD)strlen(json);
    unsigned char header[8];
    memcpy(header + 0, &op, 4);
    memcpy(header + 4, &len, 4);
    if (!PipeWrite(header, 8))
        return FALSE;
    if (len && !PipeWrite(json, len))
        return FALSE;
    return TRUE;
}

static BOOL RecvFrame(DWORD* op, char* buf, DWORD bufSize, DWORD* outLen) {
    unsigned char header[8];
    if (!PipeRead(header, 8))
        return FALSE;
    DWORD length = 0;
    memcpy(op, header + 0, 4);
    memcpy(&length, header + 4, 4);
    if (length >= bufSize)
        return FALSE;
    if (length && !PipeRead(buf, length))
        return FALSE;
    buf[length] = 0;
    if (outLen)
        *outLen = length;
    return TRUE;
}

static BOOL ConnectDiscord(void) {
    ClosePipe();
    for (int i = 0; i < 10; i++) {
        char path[64];
        _snprintf_s(path, sizeof(path), _TRUNCATE, "\\\\.\\pipe\\discord-ipc-%d", i);
        HANDLE h = CreateFileA(
            path,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        if (h == INVALID_HANDLE_VALUE)
            continue;
        g_pipe = h;

        char hs[256];
        _snprintf_s(hs, sizeof(hs), _TRUNCATE,
            "{\"v\":1,\"client_id\":\"%s\"}", DISCORD_CLIENT_ID);
        if (!SendFrame(OP_HANDSHAKE, hs)) {
            ClosePipe();
            continue;
        }
        DWORD op = 0;
        char reply[4096];
        if (!RecvFrame(&op, reply, sizeof(reply), NULL) || op == OP_CLOSE) {
            ClosePipe();
            continue;
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL SetActivity(void) {
    if (g_pipe == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD pid = GetCurrentProcessId();
    char nonce[40];
    _snprintf_s(nonce, sizeof(nonce), _TRUNCATE, "%lu-%lu", pid, GetTickCount());

    char json[1024];
    _snprintf_s(json, sizeof(json), _TRUNCATE,
        "{"
          "\"cmd\":\"SET_ACTIVITY\","
          "\"args\":{"
            "\"pid\":%lu,"
            "\"activity\":{"
              "\"details\":\"%s\","
              "\"state\":\"%s\","
              "\"instance\":true,"
              "\"timestamps\":{\"start\":%lld},"
              "\"assets\":{"
                "\"large_image\":\"%s\","
                "\"large_text\":\"%s\""
              "},"
              "\"buttons\":[{\"label\":\"%s\",\"url\":\"%s\"}]"
            "}"
          "},"
          "\"nonce\":\"%s\""
        "}",
        pid,
        RPC_DETAILS,
        RPC_STATE,
        (long long)g_start_ts,
        RPC_LARGE_IMAGE,
        RPC_LARGE_TEXT,
        RPC_BUTTON_LABEL,
        RPC_BUTTON_URL,
        nonce);

    if (!SendFrame(OP_FRAME, json))
        return FALSE;

    DWORD avail = 0;
    if (PeekNamedPipe(g_pipe, NULL, 0, NULL, &avail, NULL) && avail >= 8) {
        DWORD op = 0;
        char reply[4096];
        RecvFrame(&op, reply, sizeof(reply), NULL);
    }
    return TRUE;
}

static unsigned __stdcall RpcThread(void*) {
    Sleep(1500);

    g_start_ts = (__int64)time(NULL);

    while (!g_stop) {
        if (g_pipe == INVALID_HANDLE_VALUE) {
            if (!ConnectDiscord()) {
                Sleep(2000);
                continue;
            }
        }
        if (!SetActivity()) {
            ClosePipe();
            Sleep(1000);
            continue;
        }
        Sleep(RPC_REFRESH_MS);
    }

    ClosePipe();
    return 0;
}

void DiscordRPC_Start(void) {
    if (InterlockedCompareExchange(&g_started, 1, 0) != 0)
        return;
    g_stop = 0;
    unsigned tid = 0;
    g_thread = (HANDLE)_beginthreadex(NULL, 0, RpcThread, NULL, 0, &tid);
}

void DiscordRPC_Stop(void) {
    g_stop = 1;
    if (g_thread) {
        WaitForSingleObject(g_thread, 2000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
    ClosePipe();
    g_started = 0;
}

struct DiscordRPC_Auto {
    DiscordRPC_Auto() { DiscordRPC_Start(); }
    ~DiscordRPC_Auto() { DiscordRPC_Stop(); }
};
static DiscordRPC_Auto g_discord_rpc_auto;
