#include "alfe/main.h"

#ifndef INCLUDED_ASSERT_H
#define INCLUDED_ASSERT_H

// TODO: Posix port
// TODO: _CrtDbgBreak() is a VC-ism: port to other Windows compilers
// TODO: Use console instead of MessageBox() for console applications?

#include <CRTDBG.h>

void alert(String message, HWND hWnd = NULL)
{
    alerting = true;
    MessageBox(hWnd, NullTerminatedWideString(message), L"Error",
        MB_OK | MB_ICONERROR | MB_TASKMODAL);
    alerting = false;
}

void assert(bool success, String message = "", HWND hWnd = NULL)
{
    if (!success) {
        String output("Assertion failed");
        if (!message.empty())
            output += ": " + message;
        alert(output, hWnd);
        _CrtDbgBreak();
    }
}

#endif // INCLUDED_ASSERT_H
