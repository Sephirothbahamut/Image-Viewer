#include "image_visualizer.h"

#include "../process_event.h"
#include "../image_formats/webp.h"
#include <utils/string.h>

namespace image_panel
	{
	void image_visualizer::set_error_message() noexcept { has_error = true; }

	void image_visualizer::set_error_message(const std::u32string_view& string) noexcept
		{
		has_error = true;
		error_message.setString(string);

		const sf::Vector2f centre{error_message.getGlobalBounds().size / 2.f};
		const sf::Vector2f localBounds{centre + error_message.getLocalBounds().position};
		const sf::Vector2f rounded{std::round(localBounds.x), std::round(localBounds.y)};
		error_message.setOrigin(rounded);
		}

	image_visualizer::image_visualizer() noexcept
		{
		texture.setSmooth(smoothing);

		error_message.setLineAlignment(sf::Text::LineAlignment::Center);
		}

	bool image_visualizer::get_smooth() noexcept { return smoothing; }
	void image_visualizer::set_smooth(bool smooth) noexcept { smoothing = smooth; texture.setSmooth(smooth); }

	bool image_visualizer::try_load(const std::filesystem::path& path)
		{
		sf::Image new_image;
		bool loaded{new_image.loadFromFile(path)};
		if (!loaded)
			{
			const auto expected_image{image_formats::load_webp(path)};
			if (expected_image)
				{
				new_image = std::move(*expected_image);
				loaded = true;
				}
			}

		if (loaded && texture.loadFromImage(new_image))
			{
			logger.suc("Loaded image \"" + path.string() + "\".");
			set_error_message();
			image = new_image;
			texture.setSmooth(smoothing);
			sprite = sf::Sprite{texture};
			_current_path = path;
			return true;
			}
		else
			{
			std::u32string error{U"Failed to load image \"" + path.u32string() + U"\"."};
			set_error_message(error);
			logger.err(utils::string::cast<char>(error));
			_current_path.clear();
			image.resize({0, 0}, nullptr);
			return false;
			}
		}

	void image_visualizer::on_resize(const sf::Event::Resized& event) noexcept
		{
		const sf::Vector2u centre_u{event.size / 2u};
		const sf::Vector2f centre{static_cast<sf::Vector2f>(centre_u)};
		error_message.setPosition(centre);
		}

	bool image_visualizer::on_keyboard_pressed(const sf::Event::KeyReleased& event, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept
		{
		if (event.code == sf::Keyboard::Key::Right)
			{
			selector_handle->next(files_manager);
			return true;
			}
		return false;
		}
	bool image_visualizer::on_mouse_pressed(const sf::Event::MouseButtonPressed& event, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept
		{
		if (event.button == sf::Mouse::Button::Extra1)
			{
			selector_handle->next(files_manager);
			return true;
			}
		return false;
		}

	void image_visualizer::process_events_not_requiring_capture(const sf::Event& event) noexcept
		{
		process_event<sf::Event::Resized>(event, [&](const sf::Event::Resized& event)
			{
			on_resize(event);
			});
		}

	bool image_visualizer::process_mouse_events(const sf::Event& event, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept
		{
		return process_event<sf::Event::MouseButtonPressed>(event, [&](const sf::Event::MouseButtonPressed& event)
			{
			return on_mouse_pressed(event, files_manager, selector_handle);
			});
		}

	bool image_visualizer::process_keyboard_events(const sf::Event& event, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept
		{
		return process_event<sf::Event::KeyReleased>(event, [&](const sf::Event::KeyReleased& event)
			{
			return on_keyboard_pressed(event, files_manager, selector_handle);
			});
		}

	void image_visualizer::draw(sf::RenderTarget& rt)
		{
		rt.draw(sprite);
		if (has_error)
			{
			rt.draw(error_message);
			}
		}

	const sf::Image& image_visualizer::get_current_image() const noexcept
		{
		return image;
		}

	sf::Vector2u image_visualizer::current_image_size() const noexcept 
		{
		return texture.getSize(); 
		}

	std::filesystem::path image_visualizer::current_path() const noexcept
		{
		return _current_path;
		}
	}