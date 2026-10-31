#include "image_panel.h"

namespace image_panel
	{

	bool image_panel::is_drawing_pixel_grid() const noexcept
		{
		return mouse_drag_view.get_zoom() > draw_pixel_grid_data.minimum_zoom;
		}
	
	std::string_view default_scale::as_string(const type& default_scale_type) noexcept
		{
		return default_scale_type_strings[static_cast<size_t>(default_scale_type)];
		}
	std::string_view default_scale::as_string(const size_t& default_scale_type) noexcept
		{
		return default_scale::default_scale_type_strings[default_scale_type];
		}
	default_scale::type default_scale::from_index(const size_t& index) noexcept
		{
		return static_cast<default_scale::type>(index);
		}
	size_t default_scale::to_index(const default_scale::type& default_scale_type) noexcept
		{
		return static_cast<size_t>(default_scale_type);
		}

	image_panel::image_panel(sf::RenderTarget& render_target) noexcept :
		mouse_drag_view{render_target}
		{
		}

	image_panel::draw_pixel_grid_data_t image_panel::get_draw_pixel_grid_data() const noexcept
		{
		return draw_pixel_grid_data;
		}

	void image_panel::set_draw_pixel_grid_data(const draw_pixel_grid_data_t& new_value) noexcept
		{
		draw_pixel_grid_data = new_value;
		for (auto& vertex : vertices_array)
			{
			vertex.color = new_value.colour;
			}
		}

	void image_panel::process_events_not_requiring_capture(const sf::Event& event, sf::RenderTarget& render_target) noexcept
		{
		image_visualizer.process_events_not_requiring_capture(event);
		mouse_drag_view .process_events_not_requiring_capture(event, render_target);
		}

	bool image_panel::process_mouse_events(const sf::Event& event, sf::RenderTarget& render_target, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept
		{
		bool captured{cropper.process_mouse_events(event, render_target, image_visualizer.current_image_size())};
		captured |= mouse_drag_view.process_mouse_events(!captured, event, render_target, image_visualizer);

		if (!captured)
			{
			captured = image_visualizer.process_mouse_events(event, files_manager, selector_handle);
			}

		return captured;
		}

	bool image_panel::process_keyboard_events(const sf::Event& event, sf::RenderTarget& render_target, files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept
		{
		bool captured{cropper.process_keyboard_events(event, render_target, image_visualizer.current_image_size(), image_visualizer)};
		if (!captured)
			{
			captured = mouse_drag_view.process_keyboard_events(event, render_target, image_visualizer);
			}
		if (!captured)
			{
			captured = image_visualizer.process_keyboard_events(event, files_manager, selector_handle);
			}

		return captured;
		}

	void image_panel::draw(sf::RenderTarget& render_target) noexcept
		{
		image_visualizer.draw(render_target);

		if (is_drawing_pixel_grid())
			{
			if (mouse_drag_view.get_and_reset_view_changed()) { update_vertices_array(); }
			render_target.draw(vertices_array);
			}
		cropper.draw(render_target);
		}

	void image_panel::update_vertices_array() noexcept
		{
		const sf::View view{mouse_drag_view.get_view()};

		const sf::Vector2f ul{view.getCenter() - (view.getSize() / 2.f)};
		const sf::Vector2f dr{view.getCenter() + (view.getSize() / 2.f)};

		const sf::Vector2f outer_ul{std::floor(ul.x), std::floor(ul.y)};
		const sf::Vector2f outer_dr{std::ceil (dr.x), std::ceil (dr.y)};

		const sf::Vector2f inner_ul{std::ceil (ul.x), std::ceil (ul.y)};
		const sf::Vector2f inner_dr{std::floor(dr.x), std::floor(dr.y)};

		if (inner_ul.x >= inner_dr.x || inner_ul.y >= inner_dr.y) { vertices_array.resize(0); return; }

		const sf::Vector2u sizes{static_cast<sf::Vector2u>(inner_dr) - static_cast<sf::Vector2u>(inner_ul)};

		const auto size{(sizes.x + sizes.y) * 2u};
		vertices_array.resize(size);

		size_t i{0};

		for (float x{inner_ul.x}; x < inner_dr.x; x++)
			{
			vertices_array[i + 0].position = {x, outer_ul.y};
			vertices_array[i + 1].position = {x, outer_dr.y};
			i += 2;
			}

		for (float y{inner_ul.y}; y < inner_dr.y; y++)
			{
			vertices_array[i + 0].position = {outer_ul.x, y};
			vertices_array[i + 1].position = {outer_dr.x, y};
			i += 2;
			}

		for (auto& vertex : vertices_array)
			{
			vertex.color = draw_pixel_grid_data.colour;
			}

		assert(i == size);
		}

	bool image_panel::try_load(const std::filesystem::path& path, sf::RenderTarget& render_target)
		{
		if (!image_visualizer.try_load(path)) { return false; }


		mouse_drag_view.set_bounds(static_cast<sf::Vector2f>(image_visualizer.current_image_size()), render_target);
		cropper.clamp_rect(image_visualizer.current_image_size());
		cropper.update_output_path(image_visualizer);

		switch (current_default_scale)
			{
			case default_scale::type::keep_current: break;
			case default_scale::type::exact_pixel : change_zoom_to_exact_pixel(render_target); break;
			case default_scale::type::fit_major   : change_zoom_to_fit_major  (render_target); break;
			case default_scale::type::fit_minor   : change_zoom_to_fit_minor  (render_target); break;
			}
		return true;
		}
	
	void image_panel::change_zoom_to_exact_pixel(sf::RenderTarget& render_target) noexcept { mouse_drag_view.change_zoom_to_exact_pixel(static_cast<sf::Vector2f>(image_visualizer.current_image_size()), render_target); }
	void image_panel::change_zoom_to_fit_minor  (sf::RenderTarget& render_target) noexcept { mouse_drag_view.change_zoom_to_fit_minor  (static_cast<sf::Vector2f>(image_visualizer.current_image_size()), render_target); }
	void image_panel::change_zoom_to_fit_major  (sf::RenderTarget& render_target) noexcept { mouse_drag_view.change_zoom_to_fit_major  (static_cast<sf::Vector2f>(image_visualizer.current_image_size()), render_target); }
	void image_panel::centre_for                (sf::RenderTarget& render_target) noexcept { mouse_drag_view.centre_for                (static_cast<sf::Vector2f>(image_visualizer.current_image_size()), render_target); }
	}