// CPPTestApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/*
	Задание

	Указать те ошибки в коде, которые приводят к неправильной работе программы. При этом развернуто и подробно описать, в чем каждая из таких ошибок заключается, как проявляется, к каким именно проблемам приводит. Исправлять найденные ошибки не следует.  Про артефакты, которые не приводят к неправильной работе программы, писать не следует -- за спам снимаем баллы.
*/

#include <cstdio>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <exception>
#include <mutex>
#include <thread>

namespace TASK522
{
	typedef std::runtime_error OsError;

	//! throws an OsError exception based on errno if bResult is zero
	void Check_errno(bool bResult, const char* file, int line)
	{
		if (!bResult)
		{
			fprintf(stderr, "An error %d occurred", errno);
			std::ostringstream os;
			os << "Error with code " << errno << " occurred at " << file << "@" << line;
			throw OsError(os.str().c_str());
		};
	};

	//! check last error and throws an OsError exception if res is zero
#define TASK522_CHK_ERRNO(res)   TASK522::Check_errno(!!(res), __FILE__, __LINE__)

	//! throws an OsError exception based on errno if bResult is zero
	void Assertion(bool bResult, const char* szText, const char* file, int line)
	{
		if (!bResult)
		{
			std::ostringstream os;
			os << "Assertion " << szText << " failed at " << file << "@" << line;
			throw OsError(os.str().c_str());
		};
	};

	//! throws an exception is condition is false
#define TASK522_ASSERTION(res)   TASK522::Assertion(!!(res), #res, __FILE__, __LINE__)    

	//! Recursive synchronization object
	typedef std::recursive_mutex CriticalSection;


	//! RAII helper for the recursive synchronization object
	typedef std::lock_guard<std::recursive_mutex> AutoCriticalSection;


	//! Abstract control class
	class AsyncActionBase
	{
	public:
		virtual void DoCall() = 0;
		virtual ~AsyncActionBase() = default;
	};

	/*! Implementation of an async. call

		\tparam class that must support method 'void TT::Call()'
	*/
	template <class T>
	class AsyncActionImplT
	{
		AsyncActionImplT& operator=(const AsyncActionImplT&);
		AsyncActionImplT(const AsyncActionImplT&);
	public:
		AsyncActionImplT(AsyncActionBase* pControl)
			: m_pControl(pControl)
		{
			std::cout << "class AsyncActionImplT calls Ctor" << std::endl;
			Create();
		};

		virtual ~AsyncActionImplT()
		{
			std::cout << "class AsyncActionImplT calls Dtor" << std::endl;
			Destroy();
		};

	private:
		void Create()
		{
			std::cout << "class AsyncActionImplT calls Create" << std::endl;
			m_pThread.reset(new std::thread(&AsyncActionImplT::thread_proc, this));
		};

		void Destroy()
		{
			std::cout << "class AsyncActionImplT calls Destroy" << std::endl;
			if (m_pThread)
			{
				std::cout << "class AsyncActionImplT calls thread join" << std::endl;
				TASK522_ASSERTION(m_pThread->joinable());
				m_pThread->join();
				TASK522_ASSERTION(!m_pThread->joinable());
				m_pThread = nullptr;
			};
		};
		void thread_proc()
		{
			std::cout << "class AsyncActionImplT calls thread_proc" << std::endl;
			try
			{
				//static_cast<T*>(this)->Call();
				std::cout << "class AsyncActionImplT calls thread_proc try" << std::endl;
				if (this->m_pControl)
					this->m_pControl->DoCall();
			}
			catch (const TASK522::OsError& excpt)
			{
				std::cerr << "TASK522::OsError occured: " << excpt.what();
			}
			catch (const std::exception& excpt)
			{
				std::cerr << "Unexpected exception: " << excpt.what();
			};
		};
	private:
		std::unique_ptr<std::thread>    m_pThread;
		AsyncActionBase*                m_pControl;
	};

	//! Abstract output class
	class LoggerBase
	{
	public:
		virtual void LogLine(const char* szOut) = 0;
		virtual ~LoggerBase() = default;
	};

	//! Async action
	class AsyncAction_1
		: public AsyncActionImplT<AsyncAction_1>
	{
	public:
		AsyncAction_1(LoggerBase* pOutput, AsyncActionBase* pControl)
			: AsyncActionImplT<AsyncAction_1>(pControl)
		{
			std::cout << "class AsyncAction_1 calls Ctor" << std::endl;
			m_pOutput = pOutput;
		};

		virtual ~AsyncAction_1()
		{
			std::cout << "class AsyncAction_1 calls Dtor" << std::endl;
			m_pOutput = nullptr;
		};

		void Call()
		{
			std::cout << "class AsyncAction_1 calls Call" << std::endl;
			for (size_t i = 0; i < 100u; ++i)
			{
				std::ostringstream os;
				os << "Test1 # " << (i + 1) << std::endl;
				m_pOutput->LogLine(os.str().c_str());
			};
		};
	protected:
		LoggerBase*     m_pOutput;
	};

	//! Async action
	class AsyncAction_0
		: public AsyncActionImplT<AsyncAction_0>
	{
	public:
		AsyncAction_0(LoggerBase* pOutput, AsyncActionBase* pControl)
			: AsyncActionImplT<AsyncAction_0>(pControl)
		{
			std::cout << "class AsyncAction_0 calls Ctor" << std::endl;
			m_pOutput = pOutput;
		};

		virtual ~AsyncAction_0()
		{
			std::cout << "class AsyncAction_0 calls Dtor" << std::endl;
			m_pOutput = nullptr;
		};

		void Call()
		{
			std::cout << "class AsyncAction_0 calls Call" << std::endl;
			for (size_t i = 0; i < 1000u; ++i)
			{
				std::ostringstream os;
				os << "Test2 # " << (i + 1) << std::endl;
				m_pOutput->LogLine(os.str().c_str());
			};
		};
	protected:
		LoggerBase* m_pOutput;
	};

	class LoggerImpl
		: public LoggerBase
	{
	public:
		LoggerImpl(CriticalSection& oRM)
			: m_pOutput(nullptr)
			, m_oRM(oRM)
		{
			std::cout << "class LoggerImpl calls Ctor" << std::endl;
			m_pOutput = fopen("result.log", "w");
			TASK522_CHK_ERRNO(m_pOutput);
		};

		virtual ~LoggerImpl()
		{
			std::cout << "class LoggerImpl calls Dtor" << std::endl;
			if (m_pOutput)
			{
				fclose(m_pOutput);
				m_pOutput = nullptr;
			};
		};

		//LoggerBase
		virtual void LogLine(const char* szOut)
		{
			std::cout << "class LoggerImpl calls LogLine" << std::endl;
			AutoCriticalSection arm(m_oRM);
			if (m_pOutput)
			{
				TASK522_CHK_ERRNO(EOF != fputs(szOut, m_pOutput));
			};
		};
	protected:
		CriticalSection&            m_oRM;          // protects class variables
		FILE*               m_pOutput;
	};

	class MainImpl
		: public LoggerImpl
		, public AsyncActionBase
	{
	public:
		MainImpl()
			: LoggerImpl(m_oRM)
			, m_pAsyncAction1(nullptr)
			, m_pAsyncAction2(nullptr)
		{
			std::cout << "class MainImpl calls Ctor" << std::endl;
			Start();
		};

		virtual ~MainImpl()
		{
			std::cout << "class MainImpl calls Dtor" << std::endl;
			Destroy();
		};

		void Start()
		{
			std::cout << "class MainImpl calls Start method" << std::endl;
			AutoCriticalSection arm(m_oRM);
			m_pAsyncAction1.reset(new AsyncAction_1(this, this));
			m_pAsyncAction2.reset(new AsyncAction_0(this, this));
		};

	protected:
		void Destroy()
		{
			std::cout << "class MainImpl calls Destroy method" << std::endl;
			AutoCriticalSection arm(m_oRM);
			m_pAsyncAction1.reset();
			m_pAsyncAction2.reset();
		};


		//AsyncActionBase
		virtual void DoCall()
		{			
				std::cout << "class MainImpl calls DoCall method" << std::endl;
		};

	protected:
		CriticalSection                  m_oRM;          // protects class variables
		std::unique_ptr<AsyncAction_1>   m_pAsyncAction1;
		std::unique_ptr<AsyncAction_0>   m_pAsyncAction2;
		FILE*						     m_pOutput;
	};
};

int main()
{
	try
	{
		TASK522::MainImpl oApp;
	}
	catch (const TASK522::OsError& excpt)
	{
		std::cerr << "TASK522::OsError occured: " << excpt.what();
		return 1;
	}
	catch (const std::exception& excpt)
	{
		std::cerr << "Unexpected exception: " << excpt.what();
		return 3;
	};
	return 0;
};
