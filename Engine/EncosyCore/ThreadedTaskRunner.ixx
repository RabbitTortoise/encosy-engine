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
import <mutex>;
import <future>;
import <barrier>;


template <typename T>
class ThreadSafeQueue
{
public:
	void Push(T item)
	{
		std::scoped_lock lock(Mutex_);
		Queue_.push_back(item);
	}

	void Emplace(T&& item)
	{
		std::scoped_lock lock(Mutex_);
		Queue_.emplace_back(std::move(item));
	}

	size_t Size() 
	{
		std::scoped_lock lock(Mutex_);
		return Queue_.size();
	}

	bool Pop(T& item)
	{
		std::scoped_lock lock(Mutex_);
		if (!Queue_.empty())
		{
			if(PopFirst_)
			{
				item = std::move(Queue_.front());
				Queue_.pop_front();
				PopFirst_ = !PopFirst_;
				return true;
			}
			item = std::move(Queue_.back());
			Queue_.pop_back();
			PopFirst_ = !PopFirst_;
			return true;
		}
		return false;
	}

private:
	std::deque<T> Queue_;
	std::mutex Mutex_;
	bool PopFirst_ = true;
};


class TaskThread
{
	struct ThreadData
	{
		int ThreadId;
		std::stop_token StopToken;
		std::barrier<>* StartBarrier;
		std::barrier<>* FinishBarrier;
		std::jthread* Thread;

		ThreadSafeQueue<std::function<void()>>* WorkTaskQueue;
	};

public:
	TaskThread() {}
	~TaskThread() {}

	void CreateThread(
		std::stop_token token, 
		int id,
		std::barrier<>* startBarrier,
		std::barrier<>* finishBarrier,
		ThreadSafeQueue<std::function<void()>>* workTaskQueue
		)
	{

		Data = new ThreadData();
		Data->StopToken = token;
		Data->ThreadId = id;
		Data->StartBarrier = startBarrier;
		Data->FinishBarrier = finishBarrier;
		Data->WorkTaskQueue = workTaskQueue;
		Data->Thread = new std::jthread(std::bind_front(&TaskThread::RunThread, this), Data);

	}

	void RunThread(ThreadData* thread_data)
	{
		while (!thread_data->StopToken.stop_requested())
		{
			thread_data->StartBarrier->arrive_and_wait();

			std::function<void()> invokeFunction;
			
			while(thread_data->WorkTaskQueue->Pop(invokeFunction))
			{
				std::invoke(invokeFunction);
			}

			thread_data->FinishBarrier->arrive_and_wait();
		}
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
			ThreadCount_ = threadCountOverride;
		}
		else
		{
			ThreadCount_ = std::jthread::hardware_concurrency();
			if(ThreadCount_ < 2)
			{
				ThreadCount_ = 2;
			}
		}

		StartBarrier_ = new std::barrier(ThreadCount_ + 1);
		FinishBarrier_ = new std::barrier(ThreadCount_ + 1);

		// CreateThreads
		for (size_t i = 0; i < ThreadCount_; ++i)
		{
			Threads_.emplace_back(std::move(TaskThread()));
			Threads_[i].CreateThread(StopSource_.get_token(), i, StartBarrier_, FinishBarrier_, &WorkTaskQueue_);
		}
	}
	~ThreadedTaskRunner()
	{
		if(!ForceStopped)
		{
			ForceStopTaskRunner();
		}
		for (size_t i = 0; i < ThreadCount_; ++i)
		{
			delete Threads_[i].Data->Thread;
			delete Threads_[i].Data;
		}
		delete StartBarrier_;
		delete FinishBarrier_;
	}

	void ForceStopTaskRunner()
	{
		StopSource_.request_stop();
		auto copyToken = StartBarrier_->arrive();
		auto finishToken = FinishBarrier_->arrive();
		for (size_t i = 0; i < ThreadCount_; ++i)
		{
			Threads_[i].Data->Thread->join();
		}
		ForceStopped = true;
	}

	void RunAllTasks()
	{
		auto copyToken = StartBarrier_->arrive();
		FinishBarrier_->arrive_and_wait();
		
	}

	template <typename Function, typename... Args>
	void AddWorkTask(Function&& func, Args &&...args)
	{
		WorkTaskQueue_.Emplace(
			[f = std::forward<Function>(func), ... largs = std::forward<Args>(args)]() mutable -> decltype(auto)
			{
				std::invoke(f, largs...);
			}
		);
	}


	int GetThreadCount() { return ThreadCount_; }

private:
	int ThreadCount_ = 0;
	std::stop_source StopSource_;
	
	std::vector<TaskThread> Threads_;
	ThreadSafeQueue<std::function<void()>> WorkTaskQueue_;
	std::barrier<>* StartBarrier_;
	std::barrier<>* FinishBarrier_;

	bool ForceStopped = false;
};



class PerEntityTaskThread
{
	struct ThreadData
	{
		int ThreadId;
		std::stop_token StopToken;
		std::barrier<>* StartBarrier;
		std::barrier<>* WorkBarrier;
		std::barrier<>* SaveBarrier;
		std::barrier<>* FinishBarrier;
		std::jthread* Thread;

		ThreadSafeQueue<std::function<void()>>* CopyTaskQueue;
		ThreadSafeQueue<std::function<void()>>* WorkTaskQueue;
		ThreadSafeQueue<std::function<void()>>* SaveTaskQueue;
	};

public:
	PerEntityTaskThread() {}
	~PerEntityTaskThread() {}

	void CreateThread(
		std::stop_token token,
		int id,
		std::barrier<>* startBarrier,
		std::barrier<>* workBarrier,
		std::barrier<>* saveBarrier,
		std::barrier<>* finishBarrier,
		ThreadSafeQueue<std::function<void()>>* copyTaskQueue,
		ThreadSafeQueue<std::function<void()>>* workTaskQueue,
		ThreadSafeQueue<std::function<void()>>* saveTaskQueue)
	{

		Data = new ThreadData();
		Data->StopToken = token;
		Data->ThreadId = id;
		Data->StartBarrier = startBarrier;
		Data->WorkBarrier = workBarrier;
		Data->SaveBarrier = saveBarrier;
		Data->FinishBarrier = finishBarrier;
		Data->CopyTaskQueue = copyTaskQueue;
		Data->WorkTaskQueue = workTaskQueue;
		Data->SaveTaskQueue = saveTaskQueue;
		Data->Thread = new std::jthread(std::bind_front(&PerEntityTaskThread::RunPerEntityThread, this), Data);

	}

	void RunPerEntityThread(ThreadData* thread_data)
	{
		while (!thread_data->StopToken.stop_requested())
		{
			thread_data->StartBarrier->arrive_and_wait();

			std::function<void()> invokeFunction;
			while (thread_data->CopyTaskQueue->Pop(invokeFunction))
			{
				std::invoke(invokeFunction);
			}
			thread_data->WorkBarrier->arrive_and_wait();

			while (thread_data->WorkTaskQueue->Pop(invokeFunction))
			{
				std::invoke(invokeFunction);
			}
			thread_data->SaveBarrier->arrive_and_wait();

			while (thread_data->SaveTaskQueue->Pop(invokeFunction) && !thread_data->StopToken.stop_requested())
			{
				std::invoke(invokeFunction);
			}
			thread_data->FinishBarrier->arrive_and_wait();
		}
	}

	ThreadData* Data;
};



export class ThreadedPerEntityTaskRunner
{
public:
	explicit ThreadedPerEntityTaskRunner(int threadCountOverride = 0)
	{
		if (threadCountOverride > 0)
		{
			ThreadCount_ = threadCountOverride;
		}
		else
		{
			ThreadCount_ = std::jthread::hardware_concurrency();
			if (ThreadCount_ < 2)
			{
				ThreadCount_ = 2;
			}
		}

		StartBarrier_ = new std::barrier(ThreadCount_ + 1);
		WorkBarrier_ = new std::barrier(ThreadCount_);
		SaveBarrier_ = new std::barrier(ThreadCount_);
		FinishBarrier_ = new std::barrier(ThreadCount_ + 1);

		// CreateThreads
		for (size_t i = 0; i < ThreadCount_; ++i)
		{
			Threads_.emplace_back(std::move(PerEntityTaskThread()));
			Threads_[i].CreateThread(StopSource_.get_token(), i, StartBarrier_, WorkBarrier_, SaveBarrier_, FinishBarrier_, &CopyTaskQueue_, &WorkTaskQueue_, &SaveTaskQueue_);
		}
	}
	~ThreadedPerEntityTaskRunner()
	{
		if (!ForceStopped)
		{
			ForceStopTaskRunner();
		}
		for (size_t i = 0; i < ThreadCount_; ++i)
		{
			delete Threads_[i].Data->Thread;
			delete Threads_[i].Data;
		}
		delete StartBarrier_;
		delete WorkBarrier_;
		delete SaveBarrier_;
		delete FinishBarrier_;
	}

	void ForceStopTaskRunner()
	{
		StopSource_.request_stop();
		auto copyToken = StartBarrier_->arrive();
		auto finishToken = FinishBarrier_->arrive();
		for (size_t i = 0; i < ThreadCount_; ++i)
		{
			Threads_[i].Data->Thread->join();
		}
		ForceStopped = true;
	}

	void RunAllTasks()
	{
		auto copyToken = StartBarrier_->arrive();
		FinishBarrier_->arrive_and_wait();

	}

	template <typename Function, typename... Args>
	void AddCopyTask(Function&& func, Args &&...args)
	{
		CopyTaskQueue_.Emplace(
			[f = std::forward<Function>(func), ... largs = std::forward<Args>(args)]() mutable -> decltype(auto)
			{
				std::invoke(f, largs...);
			}
		);
	}

	template <typename Function, typename... Args>
	void AddWorkTask(Function&& func, Args &&...args)
	{
		WorkTaskQueue_.Emplace(
			[f = std::forward<Function>(func), ... largs = std::forward<Args>(args)]() mutable -> decltype(auto)
			{
				std::invoke(f, largs...);
			}
		);
	}

	template <typename Function, typename... Args>
	void AddSaveTask(Function&& func, Args &&...args)
	{
		SaveTaskQueue_.Emplace(
			[f = std::forward<Function>(func), ... largs = std::forward<Args>(args)]() mutable -> decltype(auto)
			{
				std::invoke(f, largs...);
			}
		);
	}


	int GetThreadCount() { return ThreadCount_; }

private:
	int ThreadCount_ = 0;
	std::stop_source StopSource_;

	std::vector<PerEntityTaskThread> Threads_;
	ThreadSafeQueue<std::function<void()>> CopyTaskQueue_;
	ThreadSafeQueue<std::function<void()>> WorkTaskQueue_;
	ThreadSafeQueue<std::function<void()>> SaveTaskQueue_;
	std::barrier<>* StartBarrier_;
	std::barrier<>* WorkBarrier_;
	std::barrier<>* SaveBarrier_;
	std::barrier<>* FinishBarrier_;

	bool ForceStopped = false;
};

