#include "alfe/main.h"
#include "alfe/config_file.h"
#include "alfe/windows_handle.h"

BOOL APIENTRY MyCreatePipeEx(OUT LPHANDLE lpReadPipe, OUT LPHANDLE lpWritePipe,
    IN LPSECURITY_ATTRIBUTES lpPipeAttributes, IN DWORD nSize,
    DWORD dwReadMode, DWORD dwWriteMode)
{
    HANDLE ReadPipeHandle, WritePipeHandle;
    DWORD dwError;
    WCHAR PipeNameBuffer[MAX_PATH];

    //
    //  Set the default timeout to 120 seconds
    //

    if (nSize == 0) {
        nSize = 4096;
    }

    static volatile long PipeSerialNumber;

    wsprintf(PipeNameBuffer,
        L"\\\\.\\Pipe\\ALFEAnonymousPipe.%08x.%08x",
        GetCurrentProcessId(),
        InterlockedIncrement(&PipeSerialNumber)
    );

    ReadPipeHandle = CreateNamedPipe(
        PipeNameBuffer,
        PIPE_ACCESS_INBOUND | dwReadMode,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,             // Number of pipes
        nSize,         // Out buffer size
        nSize,         // In buffer size
        120 * 1000,    // Timeout in ms
        lpPipeAttributes
    );

    if (!ReadPipeHandle) {
        return FALSE;
    }

    WritePipeHandle = CreateFile(
        PipeNameBuffer,
        GENERIC_WRITE,
        0,                         // No sharing
        lpPipeAttributes,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | dwWriteMode,
        NULL                       // Template file
    );

    if (INVALID_HANDLE_VALUE == WritePipeHandle) {
        dwError = GetLastError();
        CloseHandle(ReadPipeHandle);
        SetLastError(dwError);
        return FALSE;
    }

    *lpReadPipe = ReadPipeHandle;
    *lpWritePipe = WritePipeHandle;
    return(TRUE);
}

class Execute
{
public:
    Execute(File program, File argument, int timeout)
    {
        String commandLine = "\"" + program.path() +
            "\" \"" + argument.path() + "\"";
        NullTerminatedWideString data(commandLine);

        HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
        IF_NULL_THROW(hTimer);
        WindowsHandle hT(hTimer);

        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hChildStd_OUT_Rd;
        HANDLE hChildStd_OUT_Wr;
        IF_FALSE_THROW(
            CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0) != 0);
        WindowsHandle hChildRead = hChildStd_OUT_Rd;
        WindowsHandle hChildWrite = hChildStd_OUT_Wr;

        IF_FALSE_THROW(SetHandleInformation(hChildStd_OUT_Rd,
            HANDLE_FLAG_INHERIT, 0) != 0);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        si.hStdError = hChildStd_OUT_Wr;
        si.hStdOutput = hChildStd_OUT_Wr;
        si.hStdInput = NULL;
        si.dwFlags |= STARTF_USESTDHANDLES;

        IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, FALSE, 0, NULL,
            NULL, &si, &pi) != 0);
        CloseHandle(pi.hThread);
        WindowsHandle hProcess = pi.hProcess;

        LARGE_INTEGER dueTime;
        dueTime.QuadPart = -timeout * 10000000;
        IF_ZERO_THROW(SetWaitableTimer(hT, &dueTime, 0, NULL, NULL, FALSE));



        HANDLE handles[3] = {
            hTimer,
            hProcess,

        }

        _timedOut =
            (WaitForSingleObject(hCapture, timeout * 1000) != WAIT_OBJECT_0);
        if (_timedOut)
            IF_FALSE_THROW(TerminateProcess(pi.hProcess, 0) != 0);
        IF_FALSE_THROW(GetExitCodeProcess(pi.hProcess, &_result) != 0);
    }
    bool timedOut() { return _timedOut; }
    int result() { return _result; }
    String output() { return _output; }
private:
    bool _timedOut;
    DWORD _result;
    String _output;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        String configPath;
        if (_arguments.count() < 2)
            configPath = "harness.config";
        else
            configPath = _arguments[1];
        File file(configPath, true);
        Directory parent = file.parent();

        ConfigFile configFile;
        configFile.addOption("tests", ArrayType(StringType()));
        configFile.addOption("tool", StringType());
        configFile.addDefaultOption("baseTimeout", 5);
        configFile.addDefaultOption("expectedOutput", "PASS");
        configFile.load(file);

        auto tests = configFile.get<List<String>>("tests");
        auto tool = configFile.get<String>("tool");
        int baseTimeout = configFile.get<int>("baseTimeout");
        String expectedOutput = configFile.get<String>("expectedOutput");
        File toolFile(tool, parent);
        for (auto test : tests) {
            File f(test, parent);
            int timeout = baseTimeout;
            Execute e(tool, f, timeout);
            bool timedOut = e.timedOut();
            int result = e.result();
            String output = e.output();
            bool pass = !timedOut && result == 0 && output == expectedOutput;
            if (!pass) {
                console.write("FAIL: " + test + ": ");
                if (timedOut) {
                    console.write("timed out after " + decimal(timeout) +
                        " seconds. \n");
                }
                else {
                    if (result != 0)
                        console.write("exit code " + decimal(result) + ".\n");
                    else {
                        console.write("output was:\n" + output + "\n" +
                            "Expected:\n" + expectedOutput);
                    }
                }
                if ((timedOut || result != 0) && output != "") 
                    console.write("Output was:\n" + output + "\n");
                _returnValue = 1;
                return;
            }
        }

        console.write("PASS\n");
        _returnValue = 0;
    }
};