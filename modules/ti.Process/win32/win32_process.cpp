/**
 * Appcelerator Titanium - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2009 Appcelerator, Inc. All Rights Reserved.
 */
 
#include "win32_process.h"
#include "win32_output_pipe.h"
#include "win32_input_pipe.h"
#include <signal.h>

namespace ti
{
	AutoPtr<Win32Process> Win32Process::currentProcess = new Win32Process();
	
	/*static*/
	AutoPtr<Win32Process> Win32Process::GetCurrentProcess()
	{
		return currentProcess;
	}
	
	Win32Process::Win32Process(SharedKList args, SharedKObject environment, AutoOutputPipe stdinPipe, AutoInputPipe stdoutPipe, AutoInputPipe stderrPipe) :
		Process(args, environment, stdinPipe, stdoutPipe, stderrPipe),
		running(false),
		complete(false),
		current(false),
		pid(-1),
		exitCode(-1),
		logger(Logger::Get("Process.Win32Process"))
	{
	}
	
	Win32Process::Win32Process() :
		Process(),
		running(true), complete(false), current(true),
		logger(Logger::Get("Process.Win32Process"))
	{
		pid = GetCurrentProcessId();
		
		LPTCH env = GetEnvironmentStrings();
		while (env[0] != '\0')
		{
			std::string entry = env;
			std::string key = entry.substr(0, entry.find("="));
			std::string val = entry.substr(entry.find("=")+1);
			
			SetEnvironment(key.c_str(), val.c_str());
			env += entry.size()+1;
		}
		
		for (int i = 0; i < __argc; i++)
		{
			args->Append(Value::NewString(__argv[i]));
		}
	}
	
	Win32Process::~Win32Process()
	{
		if (!current)
		{
			try {
				Terminate();
			} catch (ValueException &ve) {
				logger->Error(ve.what());
			}
			
			if (this->exitMonitorThread.isRunning())
			{
				try
				{
					this->exitMonitorThread.join();
				}
				catch (Poco::Exception& e)
				{
					logger->Error(
						"Exception while try to join with exit monitor thread: %s",
						e.displayText().c_str());
				}
			}

			delete exitMonitorAdapter;
		}
	}
	
	/*
		Inspired by python's os.list2cmdline, ported to C++ by Marshall Culpepper
		
		Translate a sequence of arguments into a command line
		string, using the same rules as the MS C runtime:

		1) Arguments are delimited by white space, which is either a
		   space or a tab.

		2) A string surrounded by double quotation marks is
		   interpreted as a single argument, regardless of white space
		   contained within.  A quoted string can be embedded in an
		   argument.

		3) A double quotation mark preceded by a backslash is
		   interpreted as a literal double quotation mark.

		4) Backslashes are interpreted literally, unless they
		   immediately precede a double quotation mark.

		5) If backslashes immediately precede a double quotation mark,
		   every pair of backslashes is interpreted as a literal
		   backslash.  If the number of backslashes is odd, the last
		   backslash escapes the next double quotation mark as
		   described in rule 3.
		See
		http://msdn.microsoft.com/library/en-us/vccelng/htm/progs_12.asp
	*/
	std::string Win32Process::ArgListToString(SharedKList argList)
	{
		
		std::string result = "";
		bool needQuote = false;
		for (int i = 0; i < argList->Size(); i++)
		{
			std::string arg = argList->At(i)->ToString();
			std::string backspaceBuf = "";
			
			// Add a space to separate this argument from the others
			if (result.size() > 0) {
				result += ' ';
			}

			needQuote = (arg.find_first_of(" \t") != std::string::npos) || arg == "";
			if (needQuote) {
				result += '"';
			}

			for (int j = 0; j < arg.size(); j++)
			{
				char c = arg[j];
				if (c == '\\') {
					// Don't know if we need to double yet.
					backspaceBuf += c;
				}
				else if (c == '"') {
					// Double backspaces.
					result.append(backspaceBuf.size()*2, '\\');
					backspaceBuf = "";
					result.append("\\\"");
				}
				else {
					// Normal char
					if (backspaceBuf.size() > 0) {
						result.append(backspaceBuf);
						backspaceBuf = "";
					}
					result += c;
				}
			}
			// Add remaining backspaces, if any.
			if (backspaceBuf.size() > 0) {
				result.append(backspaceBuf);
			}

			if (needQuote) {
				result.append(backspaceBuf);
				result += '"';
			}
		}
		return result;
	}
	
	void Win32Process::Launch(bool async)
	{
		STARTUPINFO startupInfo;
		startupInfo.cb          = sizeof(STARTUPINFO);
		startupInfo.lpReserved  = NULL;
		startupInfo.lpDesktop   = NULL;
		startupInfo.lpTitle     = NULL;
		startupInfo.dwFlags     = STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
		startupInfo.cbReserved2 = 0;
		startupInfo.lpReserved2 = NULL;
		startupInfo.hStdInput = GetStdin()->GetReadHandle();
		startupInfo.hStdOutput = GetStdout()->GetWriteHandle();
		startupInfo.hStdError = GetStderr()->GetWriteHandle();
		
		HANDLE hProc = GetCurrentProcess();
		//GetStdin()->DuplicateRead(hProc, &startupInfo.hStdInput);
		//GetStdout()->DuplicateWrite(hProc, &startupInfo.hStdOutput);
		//GetStderr()->DuplicateWrite(hProc, &startupInfo.hStdError);
		
		std::string commandLine = ArgListToString(args);
		logger->Debug("Launching: %s", commandLine.c_str());
		PROCESS_INFORMATION processInfo;
		BOOL rc = CreateProcessA(NULL,
			(char*)commandLine.c_str(),
			NULL,
			NULL,
			TRUE,
			0,
			NULL,
			NULL,
			&startupInfo,
			&processInfo);
		// TODO auto close stdin read handle if / when piped input is finished reading
		CloseHandle(GetStdin()->GetReadHandle());
		CloseHandle(GetStdout()->GetWriteHandle());
		CloseHandle(GetStderr()->GetWriteHandle());
		
		//CloseHandle(startupInfo.hStdInput);
		//CloseHandle(startupInfo.hStdOutput);
		//CloseHandle(startupInfo.hStdError);
		
		if (!rc) {
			std::string message = "Error launching: " + commandLine;
			logger->Error(message);
			throw ValueException::FromString(message);
		}
		else {
			CloseHandle(processInfo.hThread);
			this->pid = processInfo.dwProcessId;
			this->process = processInfo.hProcess;
			this->running = true;
		}
		
		// setup threads which can read output and also monitor the exit
		GetStdout()->StartMonitor();
		GetStderr()->StartMonitor();
		
		if (async) {
			this->exitMonitorAdapter = new Poco::RunnableAdapter<Win32Process>(*this, &Win32Process::ExitMonitor);
			this->exitMonitorThread.start(*exitMonitorAdapter);
		} else {
			ExitMonitor();
		}
	}
	
	int Win32Process::GetPID()
	{
		return pid;
	}
	
	void Win32Process::Terminate()
	{
		if (running) {
			running = false;
			HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, this->pid);
			if (hProc)
			{
				if (TerminateProcess(hProc, 0) == 0)
				{
					CloseHandle(hProc);
					// not sure if this is the right thing to do
					throw ValueException::FromString("cannot kill process");
				}
				CloseHandle(hProc);
			}
			else
			{
				switch (GetLastError())
				{
				case ERROR_ACCESS_DENIED:
					throw ValueException::FromString("cannot kill process");
				case ERROR_NOT_FOUND:
					throw ValueException::FromString("cannot kill process");
				default:
					throw ValueException::FromString("cannot kill process");
				}
			}
		}
	}
	
	void Win32Process::Kill()
	{
		Terminate();
	}
	
	void Win32Process::SendSignal(int signal)
	{
		// This only works for the current process in Win32.. I guess there's nothing to do?
		if (this->current)
		{
			raise(signal);
		}
	}
	
	void Win32Process::Restart()
	{
		//TODO
	}
	
	void Win32Process::Restart(SharedKObject env, AutoOutputPipe stdinPipe, AutoInputPipe stdoutPipe, AutoInputPipe stderrPipe)
	{
		//TODO
	}
		
	void Win32Process::ExitMonitor()
	{
		
		while (true) {
			DWORD rc = WaitForSingleObject(this->process, 250);
			if (rc == WAIT_OBJECT_0) {
				break;
			}
			if (rc == WAIT_ABANDONED) {
				break;
			}
			else continue;
		}
		
		DWORD exitCode;
		if (GetExitCodeProcess(this->process, &exitCode) == 0) {
			throw ValueException::FromString("Cannot get exit code for process");
		}
		this->exitCode = exitCode;
		this->running = false;
		this->complete = true;
		
		Process::Exited();
	}
	
	bool Win32Process::IsRunning()
	{
		return running;
	}
}
