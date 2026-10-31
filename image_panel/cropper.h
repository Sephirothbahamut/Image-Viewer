#pragma once

#include <optional>

#include <SFML/Graphics.hpp>

#include "image_visualizer.h"
#include "../image_formats/webp.h"

namespace image_panel
	{
	struct crop_metadata
		{
		sf::Vector2u ul;
		sf::Vector2u dr;

		std::string name;
		std::string save_as;
		image_formats::encoder encoder;
		};

	struct cropper
		{
		private:
			std::optional<sf::Vector2u> dragging{std::nullopt};
			bool editing_mode_enabled{false};
			sf::VertexArray sfml_vertices_array{sf::PrimitiveType::LineStrip, 5};

		public:

			std::vector<crop_metadata> templates;

			std::filesystem::path output_path;

			crop_metadata current;

			void sfml_rectangle_update() noexcept;

			bool is_dragging() const noexcept;
			bool is_editing_mode_enabled() const noexcept;
			void set_editing_mode_enabled(bool enabled) noexcept;

			sf::Color get_colour() const noexcept;
			void set_colour(sf::Color new_value) noexcept;

			void clamp_rect(const sf::Vector2u& texture_size) noexcept;
			void update_output_path(const image_visualizer& image_visualizer) noexcept;

			bool on_keyboard_pressed(const sf::Event::KeyReleased& event, const sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept;

			void on_mouse_pressed(const sf::Event::MouseButtonPressed& event, const sf::RenderTarget& render_target, const sf::Vector2u& texture_size) noexcept;

			void on_mouse_moved(const sf::Event::MouseMoved& event, const sf::RenderTarget& render_target, const sf::Vector2u& texture_size) noexcept;

			void on_mouse_released(const sf::Event::MouseButtonReleased& event) noexcept;

			bool process_mouse_events(const sf::Event& event, sf::RenderTarget& render_target, const sf::Vector2u& texture_size) noexcept;
			bool process_keyboard_events(const sf::Event& event, sf::RenderTarget& render_target, const sf::Vector2u& texture_size, const image_visualizer& image_visualizer) noexcept;

			void draw(sf::RenderTarget& render_target);

			void create_output(const image_visualizer& image_visualizer) const noexcept;

			void load_templates() noexcept;
			void save_templates() noexcept;
		};
	}