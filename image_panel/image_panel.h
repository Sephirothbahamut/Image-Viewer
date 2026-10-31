#pragma once

#include <optional>

#include <SFML/Graphics.hpp>
#include "image_visualizer.h"
#include "mouse_drag_view.h"
#include "../files_manager.h"
#include "cropper.h"

namespace image_panel
	{
	namespace default_scale
		{
		enum class type { keep_current, exact_pixel, fit_minor, fit_major };
		inline static constexpr const std::array<std::string_view, 4> default_scale_type_strings{"Keep current", "Exact pixel", "Fit major axis", "Fit minor axis"};
		std::string_view as_string(const type& default_scale_type) noexcept;
		std::string_view as_string(const size_t& default_scale_type) noexcept;
		default_scale::type from_index(const size_t& index) noexcept;
		size_t to_index(const default_scale::type& default_scale_type) noexcept;
		}

	class image_panel
		{
		private:
			sf::VertexArray vertices_array{sf::PrimitiveType::Lines};

			void update_vertices_array() noexcept;

		public:
			struct draw_pixel_grid_data_t
				{
				sf::Color colour;
				float minimum_zoom{8.f};
				};
		private:
			draw_pixel_grid_data_t draw_pixel_grid_data;

		public:
			bool is_drawing_pixel_grid() const noexcept;

			default_scale::type current_default_scale{default_scale::type::fit_major};

			image_visualizer image_visualizer;
			mouse_drag_view mouse_drag_view;
			cropper cropper;

			image_panel(sf::RenderTarget& render_target) noexcept;

			draw_pixel_grid_data_t get_draw_pixel_grid_data() const noexcept;
			void set_draw_pixel_grid_data(const draw_pixel_grid_data_t& new_value) noexcept;

			void process_events_not_requiring_capture(const sf::Event& event, sf::RenderTarget& render_target) noexcept;
			bool process_mouse_events                (const sf::Event& event, sf::RenderTarget& render_target, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept;
			bool process_keyboard_events             (const sf::Event& event, sf::RenderTarget& render_target, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept;
			void draw(sf::RenderTarget& render_target) noexcept;

			bool try_load(const std::filesystem::path& path, sf::RenderTarget& render_target);

			void change_zoom_to_exact_pixel(sf::RenderTarget& render_target) noexcept;
			void change_zoom_to_fit_minor  (sf::RenderTarget& render_target) noexcept;
			void change_zoom_to_fit_major  (sf::RenderTarget& render_target) noexcept;
			void centre_for                (sf::RenderTarget& render_target) noexcept;
		};
	}