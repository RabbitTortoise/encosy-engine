module;

export module  RenderCore.RenderThread;

import <thread>;
import <functional>;
import <mutex>;
import <barrier>;
import <queue>;


template <typename T>
class ThreadSafeQueue
{
public:
	void Emplace(T&& item)
	{
		std::scoped_lock lock(Mutex_);
		Queue_.emplace(std::move(item));
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
			item = std::move(Queue_.back());
			Queue_.pop();
			PopFirst_ = !PopFirst_;
			return true;
		}
		return false;
	}

	void Clear()
	{
		std::scoped_lock lock(Mutex_);
		std::queue<T> queue;
		Queue_ = queue;
	}

private:
	std::queue<T> Queue_;
	std::mutex Mutex_;
	bool PopFirst_ = true;
};

export
class RenderThread
{
	friend class RenderCore;


	struct ThreadData
	{
		std::stop_token StopToken;
		std::barrier<>* StartBarrier;
		std::barrier<>* FinishBarrier;
		std::jthread Thread;
	};

	void CreateRenderThread(
		std::stop_token token,
		std::barrier<>* startBarrier,
		std::barrier<>* finishBarrier
		)
	{
		Data = new ThreadData();
		Data->StopToken = token;
		Data->StartBarrier = startBarrier;
		Data->FinishBarrier = finishBarrier;
		Data->Thread = std::jthread(std::bind_front(&RenderThread::RunThread, this), Data);
	}

	template <typename Function, typename... Args>
	void AddTask(Function&& func, Args &&...args)
	{
		Queue_.Emplace(
			[f = std::forward<Function>(func), ... largs = std::forward<Args>(args)]() mutable -> decltype(auto)
			{
				std::invoke(f, largs...);
			}
		);
	}

	bool GetIsWorking()
	{
		return bIsWorking;
	}

	void ClearQueue()
	{
		Queue_.Clear();
	}

private:

	void RunThread(ThreadData* threadData)
	{
		while (!threadData->StopToken.stop_requested())
		{
			threadData->StartBarrier->arrive_and_wait();

			std::function<void()> invokeFunction;
			if (Queue_.Pop(invokeFunction))
			{
				bIsWorking.exchange(true);
				std::invoke(invokeFunction);
			}
			threadData->FinishBarrier->arrive_and_wait();
			bIsWorking.exchange(false);
		}
	}

	ThreadSafeQueue<std::function<void()>> Queue_;
	std::atomic<bool> bIsWorking = false;
	ThreadData* Data;
};
