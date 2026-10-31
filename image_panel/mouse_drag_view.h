#pragma once

#include <optional>

#include <SFML/Graphics.hpp>

#include "image_visualizer.h"

namespace image_panel
	{
	struct mouse_drag_view
		{
		private:
			struct drag_metadata
				{
				sf::Vector2f starting_position_mouse;
				bool moved_at_least_one_pixel{false};
				};

			std::optional<drag_metadata> dragging;
			
			sf::View view;

			sf::Vector2f bounds{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};

			float zoom{1.f};

			bool view_changed{true};

			void apply_zoom(sf::RenderTarget& render_target) noexcept;

			void enforce_bounds(sf::RenderTarget& render_target) noexcept;

		public:
			mouse_drag_view(sf::RenderTarget& render_target) noexcept;

			bool get_and_reset_view_changed() noexcept;

			float get_zoom() const noexcept;

			void set_zoom(float new_zoom, sf::RenderTarget& render_target) noexcept;

			sf::View get_view() const noexcept;

			bool is_mouse_captured() const noexcept;

			void on_resize(const sf::Event::Resized& event, sf::RenderTarget& render_target) noexcept;

			bool on_keyboard_pressed(const sf::Event::KeyReleased& event, sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept;
			
			void on_mouse_pressed(bool enable_left, const sf::Event::MouseButtonPressed& event, const sf::RenderTarget& render_target) noexcept;

			void on_mouse_moved(const sf::Event::MouseMoved& event, sf::RenderTarget& render_target) noexcept;

			void on_mouse_released(bool enable_left, const sf::Event::MouseButtonReleased& event, sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept;

			void on_mouse_scroll_wheel(const sf::Event::MouseWheelScrolled& event, sf::RenderTarget& render_target) noexcept;

			void process_events_not_requiring_capture(const sf::Event& event, sf::RenderTarget& render_target) noexcept;
			
			/// <returns>True if mouse has been captured.</returns>
			bool process_mouse_events   (bool enable_left, const sf::Event& event, sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept;
			bool process_keyboard_events(const sf::Event& event, sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept;

			void change_zoom_to_exact_pixel(const sf::Vector2f& target_size, sf::RenderTarget& render_target) noexcept;
			void change_zoom_to_fit_minor(const sf::Vector2f& target_size, sf::RenderTarget& render_target) noexcept;
			void change_zoom_to_fit_major(const sf::Vector2f& target_size, sf::RenderTarget& render_target) noexcept;
			void centre_for(const sf::Vector2f& target_size, sf::RenderTarget& render_target) noexcept;
			void set_bounds(const sf::Vector2f& new_bounds, sf::RenderTarget& render_target) noexcept;
		};
	}