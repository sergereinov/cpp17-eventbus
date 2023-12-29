#pragma once

/*
Author: Serge Reinov
Git source: https://github.com/sergereinov/cpp17-eventbus
License: MIT
*/

#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <any>
#include <typeindex>

//inspired by ref https://github.com/gelldur/EventBus

/*
	Single-threaded c++17 event bus.

	Example:
	---

	//create event bus instance
	auto bus = std::make_shared<eventbus::EventBus>();

	//create listener instance with RAII
	auto listener = eventbus::Listener(bus);

	//declare event entity
	struct A {};

	//add callback for the event of the entity
	listener.listen<A>([](const A& entity) {
		//catch the event
	});

	//execute event immediately
	bus->immediate(A());

	//add postponded event
	bus->post(A());

	//execute all posponded events
	bus->process();

	---
*/

namespace eventbus
{
	namespace internal
	{
		// event id helper ////////////////
		using event_id_t = std::type_index;
		template <typename T>
		constexpr event_id_t event_id() { return std::type_index(typeid(T)); }
		// --------------------------------

		// vector helper //////////////////
		template<class T>
		void erase_if(std::vector<T>& v, std::function<bool(const T& item)> pred)
		{
			for (auto it = v.begin(); it != v.end();)
			{
				if (pred(*it)) it = v.erase(it);
				else ++it;
			}
		}
		template<class T>
		bool contains(const std::vector<T>& v, const T& item)
		{
			for (const T& t : v)
			{
				if (t == item) return true;
			}
			return false;
		}
		// --------------------------------

		// callback wrapper ///////////////
		class CallbackBase
		{
		public:
			virtual void execute(std::any e) const = 0;
		};

		template<class Event>
		class Callback : public CallbackBase
		{
		public:
			using callback_t = std::function<void(const Event&)>;
			Callback(callback_t cb) : cb(cb) {};
			virtual void execute(std::any e) const override
			{
				if (cb) cb(std::any_cast<Event>(e));
			}
		private:
			callback_t cb = nullptr;
		};
		// --------------------------------

	} //end of internal namespace

	class Listener;

	class EventBus
	{
		friend class Listener;

		struct ListenerCallbacks
		{
			int listener_id;
			std::vector<std::unique_ptr<internal::CallbackBase>> calls;
		};

		using EventListeners = std::vector<ListenerCallbacks>;
		using ListenersMap = std::map<internal::event_id_t, EventListeners>;

		struct Postponded
		{
			internal::event_id_t id;
			std::any event;
		};

	public:

		template <class Event>
		constexpr void immediate(Event e) const
		{
			auto id = internal::event_id<Event>();
			immediate(id, e);
		}

		template <class Event>
		void post(Event e)
		{
			auto pe = Postponded{ internal::event_id<Event>(), e };
			m_postpondedEvents.emplace_back(std::move(pe));
		}

		void process()
		{
			for (auto& pe : m_postpondedEvents)
			{
				immediate(pe.id, pe.event);
			}
			m_postpondedEvents.clear();
		}

	private:
		int m_lastListenerId = 0;
		ListenersMap m_listeners;
		std::vector<Postponded> m_postpondedEvents;

		inline void immediate(internal::event_id_t id, std::any e) const
		{
			ListenersMap::const_iterator it = m_listeners.find(id);
			if (it == m_listeners.end()) return;
			for (const auto& el : it->second)
			{
				for (const auto& c : el.calls)
				{
					c->execute(e);
				}
			}
		}

		int getNewListenerId() { return ++m_lastListenerId; }

		void removeListener(internal::event_id_t id, int listener_id)
		{
			// remove listener for event id
			auto it = m_listeners.find(id);
			if (it == m_listeners.end()) return;
			internal::erase_if<ListenerCallbacks>(it->second, [listener_id](const ListenerCallbacks& elem) {
				return elem.listener_id == listener_id;
			});
			if (it->second.empty()) m_listeners.erase(id);
		}

		void removeListener(int listener_id)
		{
			// remove listener for all event ids
			for (auto it = m_listeners.begin(); it != m_listeners.end();)
			{
				internal::erase_if<ListenerCallbacks>(it->second, [listener_id](const ListenerCallbacks& elem) {
					return elem.listener_id == listener_id;
				});
				if (it->second.empty()) it = m_listeners.erase(it);
				else it++;
			}
		}

		void addListener(internal::event_id_t id, int listener_id, std::unique_ptr<internal::CallbackBase>&& callback)
		{
			auto& eventListeners = m_listeners[id];
			auto listener = std::find_if(eventListeners.begin(), eventListeners.end(), [listener_id](const ListenerCallbacks& elem) {
				return elem.listener_id == listener_id;
			});
			if (listener == eventListeners.end())
			{
				eventListeners.push_back(ListenerCallbacks{ listener_id });
				eventListeners.rbegin()->calls.emplace_back(std::move(callback));
			}
			else listener->calls.emplace_back(std::move(callback));
		}
	};

	class Listener
	{
	public:
		Listener(const std::shared_ptr<EventBus>& bus) : m_bus(bus), m_listenerId(bus->getNewListenerId()) {}
		~Listener() { unlistenAll(); }

		template <class Event>
		void listen(std::function<void(const Event&)>&& callback)
		{
			if (!m_bus) return;
			auto id = internal::event_id<Event>();
			m_bus->addListener(id, m_listenerId, std::make_unique<internal::Callback<Event>>(callback));
		}

		template <class Event>
		void unlisten()
		{
			auto id = internal::event_id<Event>();
			if (m_bus) m_bus->removeListener(id, m_listenerId);
		}

		void unlistenAll()
		{
			if (m_bus) m_bus->removeListener(m_listenerId);
		}

	private:
		const int m_listenerId;
		std::shared_ptr<EventBus> m_bus = nullptr;
	};
}
