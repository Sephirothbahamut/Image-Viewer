#pragma once

#include <SFML/Window.hpp>

template <typename T, typename callback_t>
auto process_event(const sf::Event& event, const callback_t& callback)
	{
	const T* instance_opt{event.getIf<T>()};
	if (instance_opt)
		{
		const T& instance{*instance_opt};
		return callback(instance);
		}
	}