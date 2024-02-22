module;

#include <fmt/core.h>
#include <thread>
#include <semaphore>
#include <chrono>

export module EncosyCore.ThreadedTaskRunner;

import <atomic>;
import <queue>;
import <functional>;
import <memory>;

class TaskThread
{
	struct ThreadData
	{
		int ThreadId;
		std::stop_token StopToken;
		std::binary_semaphore* NewTaskReady;
		std::binary_semaphore* TaskFinished;
		std::jthread* Thread;
		std::function<void()> CurrentTask;
	};

public:
	TaskThread() {
		Data = new ThreadData();
		Data->NewTaskReady = new std::binary_semaphore(0);
		Data->TaskFinished = new std::binary_semaphore(1);
	}
	~TaskThread() {}

	void CreateThread(std::stop_token token, int id)
	{
		//Thread = new std::jthread(RunThread, token, id, NewTaskReady, TaskFinished);
		//Data->StopToken = std::move(token)
		Data->StopToken = token;
		Data->ThreadId = id;
		Data->Thread = new std::jthread(std::bind_front(&TaskThread::RunThread, this), Data);
	}

	void RunThread(ThreadData* thread_data)
	{
		while (!thread_data->StopToken.stop_requested())
		{
			//fmt::println("{}: Waiting for new task", thread_data->ThreadId);
			thread_data->NewTaskReady->acquire();
			if (!thread_data->StopToken.stop_requested())
			{
				//fmt::println("{}: Task Started", thread_data->ThreadId);

				//std::this_thread::sleep_for(std::chrono::duration<double>(1));
				std::invoke(thread_data->CurrentTask);

				//fmt::println("{}: Task finished", thread_data->ThreadId);
				thread_data->TaskFinished->release();
			}
		}
		//fmt::println("{}: Stop was requested", thread_data->ThreadId);
	}

	ThreadData* Data;
};


export class ThreadedTaskRunner
{
public:
	explicit ThreadedTaskRunner(int threadCountOverride = 0)
	{
		if (threadCountOverride > 0)
		{
			ThreadCount = threadCountOverride;
		}
		else
		{
			ThreadCount = std::jthread::hardware_concurrency();
			if(ThreadCount < 2)
			{
				ThreadCount = 2;
			}
		}

		// CreateThreads
		for (size_t i = 0; i < ThreadCount; ++i)
		{
			Threads.emplace_back(std::move(TaskThread()));
			Threads[i].CreateThread(StopSource.get_token(), i);
		}
	}
	~ThreadedTaskRunner()
	{

		StopSource.request_stop();
		for (size_t i = 0; i < ThreadCount; ++i)
		{
			Threads[i].Data->NewTaskReady->release();
			Threads[i].Data->TaskFinished->release();
			Threads[i].Data->Thread->join();
			delete Threads[i].Data->Thread;
			delete Threads[i].Data->NewTaskReady;
			delete Threads[i].Data->TaskFinished;
			delete Threads[i].Data;
		}
	}

	void RunAllTasks()
	{
		//Run all tasks in the queue
		while (!TaskQueue.empty())
		{
			for (size_t i = 0; i < ThreadCount; ++i)
			{
				if (Threads[i].Data->TaskFinished->try_acquire_for(std::chrono::microseconds(0)))
				{
					if (TaskQueue.empty())
					{
						Threads[i].Data->TaskFinished->release();
						continue;
					}
					auto task = TaskQueue.front();
					Threads[i].Data->CurrentTask = task;
					TaskQueue.pop();
					Threads[i].Data->NewTaskReady->release();
				}
			}
		}
		//Wait for all tasks to finish
		for (size_t i = 0; i < ThreadCount; ++i)
		{
			Threads[i].Data->TaskFinished->acquire();
			Threads[i].Data->TaskFinished->release();
		}
	}

	template <typename Function, typename... Args>
	void AddTask(Function&& func, Args &&...args)
	{
		TaskQueue.emplace(
			[f = std::forward<Function>(func), ... largs = std::forward<Args>(args)]() mutable -> decltype(auto)
			{
				std::invoke(f, largs...);
			}
		);
	}

	int GetThreadCount() { return ThreadCount; }

private:
	int ThreadCount = 0;
	std::stop_source StopSource;
	std::vector<TaskThread> Threads;
	std::queue<std::function<void()>> TaskQueue;

};
