#include "alfe/main.h"

#ifndef INCLUDED_ASSERT_H
#define INCLUDED_ASSERT_H

// TODO: Posix port
// TODO: _CrtDbgBreak() is a VC-ism: port to other Windows compilers
// TODO: Use console instead of MessageBox() for console applications?

#ifdef _WIN32
#include <CRTDBG.h>
#else
#include <signal.h>
#endif

#ifdef _WIN32
void alert(String message, HWND hWnd = NULL)
{
    alerting = true;
    MessageBox(hWnd, NullTerminatedWideString(message), L"Error",
        MB_OK | MB_ICONERROR | MB_TASKMODAL);
    alerting = false;
}
#endif

void assert(bool success, String message = ""
#ifdef _WIN32
            , HWND hWnd = NULL
#endif
            )
{
    if (!success) {
        String output("Assertion failed");
        if (!message.empty())
            output += ": " + message;
#ifdef _WIN32
        alert(output, hWnd);
        _CrtDbgBreak();
#else
        console.write(output + "\n");
        raise(SIGABRT);
#endif
    }
}

#endif // INCLUDED_ASSERT_H
