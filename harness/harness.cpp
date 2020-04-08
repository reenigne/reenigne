#include "alfe/main.h"
#include "alfe/config_file.h"
#include "alfe/windows_handle.h"
#include "alfe/named_pipe.h"
#include "alfe/thread.h"
#include "alfe/io_dispatch.h"

class Execute
{
public:
    Execute(File program, File argument, int timeout)
    {
        IODispatch dispatch;
        class PipeTask : public IODispatch::Task
        {
            const int _bufferLength = 0x10000;
        public:
            PipeTask(Execute* execute)
              : _execute(execute), _pipe("", 4096, 0, true), _overlapped{0},
                _event(true), _buffer(_bufferLength)
            {
                _pipe.read().setHandleInformation(HANDLE_FLAG_INHERIT, 0);
                _overlapped.hEvent = _event;
                startRead();
                _execute->setPipeWrite(_pipe.write());
            }
            void signalled()
            {
                DWORD bytes;
                IF_ZERO_THROW(GetOverlappedResult(_pipe.read(),
                    &_overlapped, &bytes, TRUE));
                _execute->appendOutput(String(&_buffer[0], bytes, true));
                if (GetLastError() != ERROR_HANDLE_EOF)
                    startRead();
                else
                    remove();
            }
        private:
            void startRead()
            {
                DWORD bytesRead = 0;
                BOOL r = ReadFile(_pipe.read(), &_buffer[0], _bufferLength,
                    &bytesRead, &_overlapped);
                if (r == 0)
                    IF_FALSE_THROW(GetLastError() == ERROR_IO_PENDING);
            }

            Execute* _execute;
            NamedPipe _pipe;
            OVERLAPPED _overlapped;
            Event _event;
            Array<char> _buffer;
        };
        class ProcessTask : public IODispatch::Task
        {
        public:
            ProcessTask(Execute* execute, File program, File argument)
              : _execute(execute)
            {
                String commandLine = "\"" + program.path() +
                    "\" \"" + argument.path() + "\"";
                NullTerminatedWideString data(commandLine);

                PROCESS_INFORMATION pi;
                ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

                STARTUPINFO si;
                ZeroMemory(&si, sizeof(STARTUPINFO));
                si.cb = sizeof(STARTUPINFO);
                si.hStdError = _execute->pipeWrite();
                si.hStdOutput = _execute->pipeWrite();
                si.hStdInput = NULL;
                si.dwFlags |= STARTF_USESTDHANDLES;

                IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, FALSE, 0,
                    NULL, NULL, &si, &pi) != 0);
                CloseHandle(pi.hThread);
                _handle = pi.hProcess;
            }
            void signalled()
            {
                DWORD result;
                IF_FALSE_THROW(GetExitCodeProcess(_handle, &result) != 0);
                _execute->setResult(result);
                remove();
            }
            WindowsHandle handle() { return _handle; }
        private:
            Execute* _execute;
            WindowsHandle _handle;
        };
        ProcessTask processTask(this, program, argument);
        dispatch.add(&processTask);
        class TimeoutTask : public IODispatch::Task
        {
        public:
            TimeoutTask(Execute* execute, int timeout) : _execute(execute)
            {
                HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
                IF_NULL_THROW(hTimer);
                _handle = hTimer;

                LARGE_INTEGER dueTime;
                dueTime.QuadPart = -timeout * 10000000ll;
                IF_ZERO_THROW(
                    SetWaitableTimer(_handle, &dueTime, 0, NULL, NULL, FALSE));
            }
            void signalled() { _execute->timeOut(); remove(); }
            WindowsHandle handle() { return _handle; }
        private:
            Execute* _execute;
            WindowsHandle _handle;
        };
        TimeoutTask timeOutTask(this, timeout);
        dispatch.add(&timeOutTask);
        dispatch.run();
    }
    void timeOut()
    {
        _timedOut = true;
        IF_FALSE_THROW(TerminateProcess(_process, 0) != 0);
    }
    void setResult(int result) { _result = result; }
    bool timedOut() { return _timedOut; }
    int result() { return _result; }
    String output() { return _output; }
    void appendOutput(String output) { _output += output; }
    void setPipeWrite(WindowsHandle pipeWrite) { _pipeWrite = pipeWrite; }
    WindowsHandle pipeWrite() { return _pipeWrite; }
private:
    bool _timedOut;
    int _result;
    String _output;
    WindowsHandle _process;
    WindowsHandle _pipeWrite;
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
        configFile.addDefaultOption("expectedOutput", String("PASS"));
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