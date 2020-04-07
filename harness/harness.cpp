#include "alfe/main.h"
#include "alfe/config_file.h"
#include "alfe/windows_handle.h"
#include "alfe/named_pipe.h"
#include "alfe/thread.h"

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

        NamedPipe pipe;
        pipe.read().setHandleInformation(HANDLE_FLAG_INHERIT, 0);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        si.hStdError = pipe.write();
        si.hStdOutput = pipe.write();
        si.hStdInput = NULL;
        si.dwFlags |= STARTF_USESTDHANDLES;

        IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, FALSE, 0, NULL,
            NULL, &si, &pi) != 0);
        CloseHandle(pi.hThread);
        WindowsHandle hProcess = pi.hProcess;

        LARGE_INTEGER dueTime;
        dueTime.QuadPart = -timeout * 10000000;
        IF_ZERO_THROW(SetWaitableTimer(hT, &dueTime, 0, NULL, NULL, FALSE));

        OVERLAPPED overlapped = { 0 };
        Event event(true);
        overlapped.hEvent = event;
        int bufferLength = 0x10000;
        DWORD bytesRead = 0;
        Array<char> buffer(bufferLength);
        BOOL rr = ReadFile(pipe.read(), &buffer[0], bufferLength,
            &bytesRead, &overlapped);
        if (rr == 0)
            IF_FALSE_THROW(GetLastError() == ERROR_IO_PENDING);

        HANDLE handles[3] = {
            hTimer,
            hProcess,
            event
        };
        int events = 3;

        do {
            int r = WaitForMultipleObjects(events, &handles[0], FALSE,
                INFINITE);
            switch (r) {
                case 0:
                    _timedOut = true;
                    IF_FALSE_THROW(TerminateProcess(hProcess, 0) != 0);
                    break;
                case 1:
                    IF_FALSE_THROW(
                        GetExitCodeProcess(pi.hProcess, &_result) != 0);
                    break;
                case 2:
                    DWORD bytes;
                    IF_ZERO_THROW(GetOverlappedResult(pipe.read(),
                        &overlapped, &bytes, TRUE));
                    _output += String(&buffer[0], bytes, true);
                    if (GetLastError() != ERROR_HANDLE_EOF) {
                        BOOL rr = ReadFile(pipe.read(), &buffer[0],
                            bufferLength, &bytesRead, &overlapped);
                        if (rr == 0)
                            IF_FALSE_THROW(GetLastError() == ERROR_IO_PENDING);
                    }
                    break;
            }
        } while (true);
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