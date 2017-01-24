//
//  su_job.h
//  sutils
//
//  Created by Sandy Martel on 12/03/04.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
//	Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
//	granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
//	implied or otherwise.
//

#ifndef H_SU_JOB
#define H_SU_JOB

#include <memory>
#include <functional>
#include <condition_variable>

namespace su
{

class jobdispatcher;
class job_cancelled{};

class job : public std::enable_shared_from_this<job>
{
  public:
	virtual ~job();
	
	/*!
		@brief cancel the job.

			Cancel a job.
			-If it's waiting in the async queue, it will be removed and deleted right away.
			-If it's executing async, it will receive a thread interrupt and
				be deleted when done, it will not execute at idle.
			-If it's waiting in the idle queue, it will be removed and deleted right away.
			MUST be called from the main thread.
	*/
	void cancel();

	inline void cancellationPoint() const
	{
		if ( _cancelled.load() )
			throw job_cancelled();
	}
	
	/*!
		@brief finish a job right away.

			This will block until the job is complete.
			-If it's waiting in the async queue, it will be removed and executed right away,
				the completion will be executed as well and then it will be deleted.
			-If it's executing async, it will block until it finished, then 
				the completion will be executed as well and then it will be deleted.
			-If it's waiting in the idle queue, it will be removed, executed and deleted right away.
			MUST be called from the main thread.
	*/
	void sprint();
	
	/*!
		@brief prioritise a job.
			
			If the job is in the async queue, it will be put at the top for
			earlier execution.
			MUST be called from the main thread.
	*/
	void prioritise();
	
  protected:
	job() = default;
	
	//! will be called on a thread
	virtual void runAsync();
	
	//! will be called at idle time
	virtual void runIdle();

  private:
	jobdispatcher *_dispatcher = nullptr; //!< the dispather, null unless the job is posted
	int _threadIndex = -1; //!< -1 unless the job is running async
	std::atomic_bool _cancelled{ false };
	
	std::condition_variable _cond; // to notify termination of async jobs when cancelled
	
	friend class jobdispatcher;
	
	// forbidden
	job( const job & ) = delete;
	job &operator=( const job & ) = delete;
};

using job_ptr = std::shared_ptr<job>;
using job_weak_ptr = std::weak_ptr<job>;

/*!
   @brief Job that can run C++ lambdas
*/
class asyncJob : public job
{
  public:
	asyncJob( const std::function< void(job*) > &i_async, const std::function< void(job*) > &i_idle );
	virtual ~asyncJob();

  private:
	std::function< void(job*) > _async, _idle;
	
	virtual void runAsync();
	virtual void runIdle();
};

}

#endif
