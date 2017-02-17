//
//  su_jobdispatcher.h
//  sutils
//
//  Created by Sandy Martel on 12/03/04.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
//	Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
//	granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
//	implied or otherwise.
//

#ifndef H_SU_JOBDISPATCHER
#define H_SU_JOBDISPATCHER

#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace su
{

class job;
using job_ptr = std::shared_ptr<job>;

class jobdispatcher
{
	public:
		/*!
		   @brief Convinience to access the main dispatcher.

		       Not actually implemented.
		   @return     the main job dispatcher
		*/
		static jobdispatcher *instance();
		
		/*!
		   @brief Constructor for a dispatcher.

		       argument is the number of threads.
			   Default to number of cpu - 1.
		   @param[in] i_nbOfWorkers nb of workers
		*/
		jobdispatcher( int i_nbOfWorkers = -1 );
		virtual ~jobdispatcher();
		
		/*!
		   @brief Post a job to be executed.

		       Ownership of the job is passed to the jobdispatcher.
			   The job will be executed on a thread, then completed at idle
			   time, then deleted.
		   @param[in] i_job the job
		*/
		void postAsync( const job_ptr &i_job );

		/*!
		   @brief Post a job to be executed.

		       Ownership of the job is passed to the jobdispatcher.
			   The job will be executed at idle time, then deleted.
		   @param[in] i_job the job
		*/
		void postIdle( const job_ptr &i_job );
		
		/*!
		   @brief must be called to execute the idle jobs.
		   
				all idle jobs will be executed.
		*/
		void idle();
		
		/*!
		   @brief called to notify a subclass that idle time is needed.
		   
				May be called from any thread!
		*/
		virtual void needIdleTime();

	private:
		bool _isRunning = true;	//!< dispatcher state
		int _nbOfWorkersRunning = 0, _nbOfWorkersFree = 0;
		std::vector<std::thread *> _threadPool;

		// Async queue
		std::mutex _JDMutex;
		std::condition_variable _JDCond;
		std::list<job_ptr> _asyncQueue;
		
		// Idle queue
		std::list<job_ptr> _idleQueue;

		bool removeFromQueue( std::list<job_ptr> &io_queue, const job_ptr &i_job );
		bool moveToFrontOfQueue( std::list<job_ptr> &io_queue, const job_ptr &i_job );

		void workerThread( int i_threadIndex );

		/*!
		   @brief cancel the job.

		       Cancel a job. This will block until the job is complete.
			   -If it's waiting in the async queue, it will be removed and deleted right away.
			   -If it's executing async, it will receive a thread interrupt and
			      be deleted when the interrupt is handled.
			   -If it's waiting in the idle queue, it will be removed and deleted right away.
			   MUST be called from the main thread.
		*/
		void cancel_impl( const job_ptr &i_job );
		void cancel_with_lock_impl( const job_ptr &i_job );

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
		void sprint_impl( const job_ptr &i_job );

		/*!
			@brief prioritise a job.
				
				If the job is in the async queue, it will be put at the top for
				earlier execution.
		*/
		void prioritise_impl( const job_ptr &i_job );
	
		friend class job;
		
		// forbidden
		jobdispatcher( const jobdispatcher & ) = delete;
		jobdispatcher &operator=( const jobdispatcher & ) = delete;
};

}

#endif
