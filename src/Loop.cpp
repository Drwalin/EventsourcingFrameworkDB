/*
 *  This file is part of DORPC. Please see README for details.
 *  Copyright (C) 2021-2022 Marek Zalewski aka Drwalin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <libusockets.h>

#include "Loop.hpp"

#include "Debug.hpp"

namespace net {
	Loop::~Loop() {
		// TODO force close loop running thread
		delete *(std::shared_ptr<Loop>**)us_loop_ext(loop);
		*(std::shared_ptr<Loop>**)us_loop_ext(loop) = NULL;
	}
	
	std::shared_ptr<Loop>& Loop::ThisThreadLoop() {
		static thread_local std::shared_ptr<Loop> thread_loop = NULL;
		return thread_loop;
	}

	void Loop::Run() {
		if(ThisThreadLoop() != NULL)
			throw "Cannot run loop on thread with already running loop.";
		running = true;
		ThisThreadLoop() = self.lock();
		us_loop_run(loop);
		ThisThreadLoop() = NULL;
		running = false;
	}
	
	
	void Loop::AllcastSend(Buffer& sendBuffer, std::shared_ptr<Socket> ignore) {
		Event* event = Event::Allocate();
		event->after = NULL;
		event->buffer_or_ip = std::move(sendBuffer);
		event->type = Event::ALLCAST_LOOP;
		event->socket = ignore;
		event->loop = self.lock();
		if(this == Loop::ThisThreadLoop().get())
			event->Run();
		else
			this->PushEvent(event);
	}
	

	void Loop::PushEvent(Event* event) {
		DeferEvent(0, event);
	}

	void Loop::DeferEvent(int defer, Event* event) {
		event->defer = defer;
		events.push(event);
		us_wakeup_loop(loop);
	}

	void Loop::PopEvents() {
		Event* event;
		std::vector<Event*> deferedEvents;
		while((event = events.pop()) != NULL) {
			if(event->defer > 0) {
				event->defer--;
				deferedEvents.emplace_back(event);
			} else {
				if(event)
					event->Run();
				else
					break;
				Event::Free(event);
			}
		}
		for(size_t i=0; i<deferedEvents.size(); ++i) {
			events.push(deferedEvents[i]);
		}
		deferedEvents.clear();
	}

	void Loop::OnWakeup() {
		PopEvents();
	}

	void Loop::OnPre() {
		PopEvents();
	}

	void Loop::OnPost() {
		PopEvents();
	}

	void Loop::InternalOnWakeup(struct us_loop_t* loop) {
		(**((std::shared_ptr<Loop>**)us_loop_ext(loop)))->OnWakeup();
	}

	void Loop::InternalOnPre(struct us_loop_t* loop) {
		(**((std::shared_ptr<Loop>**)us_loop_ext(loop)))->OnPre();
	}

	void Loop::InternalOnPost(struct us_loop_t* loop) {
		(**((std::shared_ptr<Loop>**)us_loop_ext(loop)))->OnPost();
	}

	std::shared_ptr<Loop> Loop::Make() {
		struct us_loop_t* us_loop = us_create_loop(0, InternalOnWakeup,
				InternalOnPre, InternalOnPost, sizeof(std::shared_ptr<Loop>*));
		std::shared_ptr<Loop> loop(new Loop());
		*((std::shared_ptr<Loop>**)us_loop_ext(us_loop)) =
			new std::shared_ptr<Loop>(loop);
		loop->loop = us_loop;
		loop->userData = NULL;
		loop->running = false;
		loop->self = loop;
		return loop;
	}
}

