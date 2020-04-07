#include "alfe/main.h"

#ifndef INCLUDED_EMAIL_H
#define INCLUDED_EMAIL_H

void sendMail(String from, String to, String subject, String body)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE pipeReadHandle;
    HANDLE pipeWriteHandle;
    if (CreatePipe(
        &pipeReadHandle, &pipeWriteHandle, &sa, 0) == 0) {
        throw Exception::systemError("Could not create pipe");
    }

    Stream pipeRead(pipeReadHandle, File(), true);
    Stream pipeWrite(pipeWriteHandle, File(), true);

    if (SetHandleInformation(pipeWriteHandle, HANDLE_FLAG_INHERIT, 0) == 0)
        throw Exception::systemError("Could not set write handle of input "
        "pipe to not inherited");

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = console;
    si.hStdOutput = console;
    si.hStdInput = pipeReadHandle;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Make a copy of the command line because CreateProcess needs to modify
    // it.
    WCHAR commandLine[] = L"sendmail.exe -t";

    if (CreateProcess(NULL,   // lpApplicationName
        commandLine,          // lpCommandName
        NULL,                 // lpProcessAttributes
        NULL,                 // lpThreadAttributes
        TRUE,                 // bInheritHandles
        0,                    // dwCreationFlags
        NULL,                 // lpEnvironment
        NULL,                 // lpCurrentDirectory
        &si,                  // lpStartupInfo
        &pi) == 0) {          // lpProcessInformation
        throw Exception::systemError("Could not create sendmail process");
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    pipeWrite.write("From: " + from + "\n");
    pipeWrite.write("To: " + to + "\n");
    pipeWrite.write("Subject: " + subject + "\n\n");
    pipeWrite.write(body + "\n");
}

#endif // INCLUDED_EMAIL_H
