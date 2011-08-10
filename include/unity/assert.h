#ifndef INCLUDED_ASSERT_H
#define INCLUDED_ASSERT_H

// TODO: Posix port
// TODO: _CrtDbgBreak() is a VC-ism: port to other Windows compilers
// TODO: Use console instead of MessageBox() for console applications?

#include "unity/string.h"
#include <CRTDBG.h>

void alert(String message, HWND hWnd = NULL)
{
    Array<WCHAR> data;
    message.copyToUTF16(&data);
    MessageBox(hWnd, &data[0], L"Error", MB_OK | MB_ICONERROR);
}

void assert(bool success, String message = "", HWND hWnd = NULL)
{
    if (!success) {
        String output("Assertion failed");
        if (message.length() > 0)
            output += String(": ") + message;
        alert(output, hWnd);
        _CrtDbgBreak();
    }
}

#endif // INCLUDED_ASSERT_H
