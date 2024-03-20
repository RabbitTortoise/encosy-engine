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
		std::unique_lock lock(Mutex_);
		Queue_.push(item);
	}

	void Emplace(T&& item)
	{
		std::unique_lock lock(Mutex_);
		Queue_.emplace(std::move(item));
	}

	size_t Size() 
	{
		std::unique_lock lock(Mutex_);
		return Queue_.size();
	}

	bool Pop(T& item)
	{
		std::unique_lock lock(Mutex_);
		if (!Queue_.empty())
		{
			item = std::move(Queue_.front());
			Queue_.pop();
			return true;
		}

		return false;
	}


private:
	std::queue<T> Queue_;
	std::mutex Mutex_;
};

class TaskThread
{
	struct ThreadData
	{
		int ThreadId;
		std::stop_token StopToken;
		std::barrier<>* CopyBarrier;
		std::barrier<>* WorkBarrier;
		std::barrier<>* SaveBarrier;
		std::barrier<>* FinishBarrier;
		std::jthread* Thread;

		ThreadSafeQueue<std::move_only_function<void()>>* CopyTaskQueue;
		ThreadSafeQueue<std::move_only_function<void()>>* WorkTaskQueue;
		ThreadSafeQueue<std::move_only_function<void()>>* SaveTaskQueue;
	};

public:
	TaskThread() {}
	~TaskThread() {}

	void CreateThread(
		std::stop_token token, 
		int id,
		std::barrier<>* copyBarrier,
		std::barrier<>* workBarrier,
		std::barrier<>* saveBarrier,
		std::barrier<>* finishBarrier,
		ThreadSafeQueue<std::move_only_function<void()>>* copyTaskQueue,
		ThreadSafeQueue<std::move_only_function<void()>>* workTaskQueue,
		ThreadSafeQueue<std::move_only_function<void()>>* saveTaskQueue)
	{

		Data = new ThreadData();
		Data->StopToken = token;
		Data->ThreadId = id;
		Data->CopyBarrier = copyBarrier;
		Data->WorkBarrier = workBarrier;
		Data->SaveBarrier = saveBarrier;
		Data->FinishBarrier = finishBarrier;
		Data->CopyTaskQueue = copyTaskQueue;
		Data->WorkTaskQueue = workTaskQueue;
		Data->SaveTaskQueue = saveTaskQueue;
		Data->Thread = new std::jthread(std::bind_front(&TaskThread::RunThread, this), Data);

	}

	void RunThread(ThreadData* thread_data)
	{
		while (!thread_data->StopToken.stop_requested())
		{
			//fmt::println("{}: Waiting for new tasks to be ready", thread_data->ThreadId);
			thread_data->CopyBarrier->arrive_and_wait();
			if (thread_data->StopToken.stop_requested()) { return;; }

			std::move_only_function<void()> invokeFunction;
			while (thread_data->CopyTaskQueue->Pop(invokeFunction))
			{
				std::invoke(invokeFunction);
			}
			//fmt::println("{}: Waiting for other tasks to finish copying", thread_data->ThreadId);
			thread_data->WorkBarrier->arrive_and_wait();
			if (thread_data->StopToken.stop_requested()) { return; }

			while(thread_data->WorkTaskQueue->Pop(invokeFunction))
			{
				std::invoke(invokeFunction);
			}
			//fmt::println("{}: Waiting for other tasks to finish work", thread_data->ThreadId);
			thread_data->SaveBarrier->arrive_and_wait();
			if (thread_data->StopToken.stop_requested()) { return;; }

			while (thread_data->SaveTaskQueue->Pop(invokeFunction) && !thread_data->StopToken.stop_requested())
			{
				std::invoke(invokeFunction);
			}
			//fmt::println("{}: Waiting for other tasks to finish saving", thread_data->ThreadId);
			thread_data->FinishBarrier->arrive_and_wait();
		}
		//fmt::println("{}: Stop was requested, quitting thread", thread_data->ThreadId);
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

		CopyBarrier_ = new std::barrier(ThreadCount_ + 1);
		WorkBarrier_ = new std::barrier(ThreadCount_);
		SaveBarrier_ = new std::barrier(ThreadCount_);
		FinishBarrier_ = new std::barrier(ThreadCount_ + 1);

		// CreateThreads
		for (size_t i = 0; i < ThreadCount_; ++i)
		{
			Threads_.emplace_back(std::move(TaskThread()));
			Threads_[i].CreateThread(StopSource_.get_token(), i, CopyBarrier_, WorkBarrier_, SaveBarrier_, FinishBarrier_, &CopyTaskQueue_, &WorkTaskQueue_, &SaveTaskQueue_);
		}
	}
	~ThreadedTaskRunner()
	{

		StopSource_.request_stop();
		auto copyToken = CopyBarrier_->arrive();
		auto workToken = WorkBarrier_->arrive();
		auto saveToken = SaveBarrier_->arrive();
		auto finishToken = FinishBarrier_->arrive();

		for (size_t i = 0; i < ThreadCount_; ++i)
		{
			Threads_[i].Data->Thread->join();
			delete Threads_[i].Data->Thread;
			delete Threads_[i].Data;
		}
		delete CopyBarrier_;
		delete WorkBarrier_;
		delete SaveBarrier_;
		delete FinishBarrier_;
	}

	void RunAllTasks()
	{
		if(WorkTaskQueue_.Size() > 0 || SaveTaskQueue_.Size() > 0)
		{
			auto copyToken = CopyBarrier_->arrive();
			FinishBarrier_->arrive_and_wait();
		}
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
	
	std::vector<TaskThread> Threads_;
	ThreadSafeQueue<std::move_only_function<void()>> CopyTaskQueue_;
	ThreadSafeQueue<std::move_only_function<void()>> WorkTaskQueue_;
	ThreadSafeQueue<std::move_only_function<void()>> SaveTaskQueue_;
	std::barrier<>* CopyBarrier_;
	std::barrier<>* WorkBarrier_;
	std::barrier<>* SaveBarrier_;
	std::barrier<>* FinishBarrier_;
};

