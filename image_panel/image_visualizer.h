#pragma once

#include <optional>

#include <SFML/Graphics.hpp>

#include "../files_manager.h"
#include "../global.h"

namespace image_panel
	{
	class image_visualizer
		{
		private:
			std::filesystem::path _current_path;
			sf::Image image;
			sf::Texture texture;
			sf::Sprite sprite{texture};
			bool smoothing{false};

			bool has_error{false};
			sf::Text error_message{default_font};

			void set_error_message() noexcept;
			void set_error_message(const std::u32string_view& string) noexcept;

		public:
			image_visualizer() noexcept;

			bool get_smooth() noexcept;
			void set_smooth(bool smooth) noexcept;

			bool try_load(const std::filesystem::path& path);

			void on_resize(const sf::Event::Resized& event) noexcept;

			bool on_keyboard_pressed(const sf::Event::KeyReleased       & event, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept;
			bool on_mouse_pressed   (const sf::Event::MouseButtonPressed& event, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept;

			void process_events_not_requiring_capture(const sf::Event& event) noexcept;
			bool process_mouse_events   (const sf::Event& event, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept;
			bool process_keyboard_events(const sf::Event& event, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept;

			void draw(sf::RenderTarget& rt);

			std::string evaluate_variable(const std::string_view& variable_name) const noexcept;
			std::string evaluate_template_string(const std::string_view& template_string) const noexcept;

			const sf::Image&      get_current_image () const noexcept;
			sf::Vector2u          current_image_size() const noexcept;
			std::filesystem::path current_path      () const noexcept;
		};
	}