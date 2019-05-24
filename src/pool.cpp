/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "pool.h"
#include <iostream>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

io_context_pool::io_context_pool(std::size_t pool_size)
  : next_io_context_(0)
{
  if (pool_size == 0)
    throw std::runtime_error("io_context_pool size is 0");

  // Give all the io_contexts work to do so that their run() functions will not
  // exit until they are explicitly stopped.
  for (std::size_t i = 0; i < pool_size; ++i)
  {
    io_context_ptr io_context(new boost::asio::io_context);
    io_contexts_.push_back(io_context);
    work_.push_back(boost::asio::make_work_guard(*io_context));
  }
}

void io_context_pool::run()
{
	std::vector<std::thread> v;
    v.reserve(io_contexts_.size() - 1);
    for(auto i = io_contexts_.size() - 1; i > 0; --i)
    for(unsigned int i = 0; i < io_contexts_.size(); i++) {
        v.emplace_back(
        [this, i]
        {
			GC_stack_base sb;
			GC_get_stack_base(&sb);
			GC_register_my_thread(&sb);
			for (;;) {
				try {
					io_contexts_[i]->run();
					break;
				} catch (std::exception& e) {
					std::cout << "IOS failure: " << e.what() << std::endl;
					io_contexts_[i]->restart();
				}
			}
            GC_unregister_my_thread();
        });
    }
	while(1) { sleep(200); }
}

void io_context_pool::stop()
{
  // Explicitly stop all io_contexts.
  for (std::size_t i = 0; i < io_contexts_.size(); ++i)
    io_contexts_[i]->stop();
}

boost::asio::io_context& io_context_pool::get_io_context()
{
  // Use a round-robin scheme to choose the next io_context to use.
  boost::asio::io_context& io_context = *io_contexts_[next_io_context_];
  ++next_io_context_;
  if (next_io_context_ == io_contexts_.size())
    next_io_context_ = 0;
  return io_context;
}
