// Copyright (c) 2019 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef PDM_THREAD_POOL_H
#define PDM_THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include "PdmLogUtils.h"

class PdmThreadPool {

private:
    bool terminate;
    std::vector< std::thread > workers;
    std::condition_variable condition;
    std::mutex queue_mutex;
    std::queue< std::function<void()> > pdm_tasks;

public:
    PdmThreadPool(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~PdmThreadPool();

};


inline PdmThreadPool::PdmThreadPool(size_t threads)
    :   terminate(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->terminate || !this->pdm_tasks.empty(); });
                        if(this->terminate && this->pdm_tasks.empty())
                            return;
                        task = std::move(this->pdm_tasks.front());
                        this->pdm_tasks.pop();
                    }

                    task();
                }
            }
        );
}


template<class F, class... Args>
auto PdmThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if(terminate)
            throw std::runtime_error("enqueue on terminated PdmThreadPool");

        pdm_tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

inline PdmThreadPool::~PdmThreadPool()
{
    {
        try {
            std::unique_lock<std::mutex> lock(queue_mutex);
            terminate = true;
        }
        catch(const std::system_error& e) {
            PDM_LOG_ERROR("Exception occured : %s", e.what());
        }
    }
    condition.notify_all();
    for(std::thread &worker: workers) {
        if (worker.joinable()) {
            try {
                worker.join();
            }
            catch (std::exception &e) {
                PDM_LOG_ERROR("Exception occurred : %s", e.what());
            }
        }
    }
}

#endif  //PDM_THREAD_POOL_H
