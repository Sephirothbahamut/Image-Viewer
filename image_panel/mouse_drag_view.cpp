#include "mouse_drag_view.h"

#include "../process_event.h"
#include "../global.h"


namespace image_panel
	{
	mouse_drag_view::mouse_drag_view(sf::RenderTarget& render_target) noexcept
		{
		apply_zoom(render_target);
		}

	void mouse_drag_view::apply_zoom(sf::RenderTarget& render_target) noexcept
		{
		const auto viewport_size{static_cast<sf::Vector2f>(render_target.getSize())};
		view.setSize(viewport_size / zoom);
		enforce_bounds(render_target);
		}

	void mouse_drag_view::set_zoom(float new_zoom, sf::RenderTarget& render_target) noexcept
		{
		zoom = new_zoom;
		apply_zoom(render_target);
		}

	bool mouse_drag_view::get_and_reset_view_changed() noexcept
		{
		const bool ret{view_changed};
		view_changed = false;
		return ret;
		}

	float mouse_drag_view::get_zoom() const noexcept { return zoom; }
	sf::View mouse_drag_view::get_view() const noexcept { return view; };

	void mouse_drag_view::enforce_bounds(sf::RenderTarget& render_target) noexcept
		{
		auto centre{view.getCenter()};
		centre.x = std::clamp(centre.x, 0.f, bounds.x);
		centre.y = std::clamp(centre.y, 0.f, bounds.y);
		view.setCenter(centre);
		render_target.setView(view);
		view_changed = true;
		}

	bool mouse_drag_view::is_mouse_captured() const noexcept { return dragging.has_value(); }

	void mouse_drag_view::on_resize(const sf::Event::Resized& event, sf::RenderTarget& render_target) noexcept
		{
		apply_zoom(render_target);
		}

	bool mouse_drag_view::on_keyboard_pressed(const sf::Event::KeyReleased& event, sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept
		{
		if (event.code == sf::Keyboard::Key::Space)
			{
			if (event.control)  { change_zoom_to_fit_minor  (static_cast<sf::Vector2f>(image_visualizer.current_image_size()), render_target); }
			else if (event.alt) { change_zoom_to_exact_pixel(static_cast<sf::Vector2f>(image_visualizer.current_image_size()), render_target); }
			else                { change_zoom_to_fit_major  (static_cast<sf::Vector2f>(image_visualizer.current_image_size()), render_target); }
			return true;
			}

		if (event.control && !event.alt)
			{
			const float delta{32.f / get_zoom()};
			switch (event.code)
				{
				case sf::Keyboard::Key::Left : view.setCenter(view.getCenter() + sf::Vector2f{-delta, -  0.f}); enforce_bounds(render_target); return true;
				case sf::Keyboard::Key::Up   : view.setCenter(view.getCenter() + sf::Vector2f{-  0.f, -delta}); enforce_bounds(render_target); return true;
				case sf::Keyboard::Key::Right: view.setCenter(view.getCenter() + sf::Vector2f{+delta, +  0.f}); enforce_bounds(render_target); return true;
				case sf::Keyboard::Key::Down : view.setCenter(view.getCenter() + sf::Vector2f{+  0.f, +delta}); enforce_bounds(render_target); return true;
				}
			}

		const int plus_minus_delta{(event.code == sf::Keyboard::Key::Add ? 1 : 0) - (event.code == sf::Keyboard::Key::Subtract ? -1 : 0)};
		if (plus_minus_delta != 0)
			{
			const float new_zoom{std::max(get_zoom() * (plus_minus_delta > 0 ? 1.1f : 0.9f), .01f)};
			set_zoom(new_zoom, render_target);
			return true;
			}

		return false;
		}

	void mouse_drag_view::on_mouse_pressed(bool enable_left, const sf::Event::MouseButtonPressed& event, const sf::RenderTarget& render_target) noexcept
		{
		if ((event.button == sf::Mouse::Button::Right) || (enable_left && (event.button == sf::Mouse::Button::Left)))
			{
			const auto mouse_position_in_world{render_target.mapPixelToCoords(event.position)};
			dragging = {.starting_position_mouse{mouse_position_in_world}};
			}
		}

	void mouse_drag_view::on_mouse_moved(const sf::Event::MouseMoved& event, sf::RenderTarget& render_target) noexcept
		{
		if (dragging)
			{
			const auto mouse_position_in_world{render_target.mapPixelToCoords(event.position)};
			const auto delta{dragging->starting_position_mouse - mouse_position_in_world};
			if ((delta.x == 0.f) || (delta.y == 0.f)) { return; }

			view.move(delta);
			render_target.setView(view);
			const auto updated_mouse_position_in_world{render_target.mapPixelToCoords(event.position)};
			dragging->starting_position_mouse = updated_mouse_position_in_world;
			dragging->moved_at_least_one_pixel = true;
			enforce_bounds(render_target);
			}
		}

	void mouse_drag_view::on_mouse_released(bool enable_left, const sf::Event::MouseButtonReleased& event, sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept
		{
		const bool left {enable_left && (event.button == sf::Mouse::Button::Left )};
		const bool right{                event.button == sf::Mouse::Button::Right };
		if (left || right)
			{
			const auto mouse_position_in_world{render_target.mapPixelToCoords(event.position)};
			if (dragging && enable_left && right && !dragging->moved_at_least_one_pixel)
				{
				change_zoom_to_fit_major(static_cast<sf::Vector2f>(image_visualizer.current_image_size()), render_target);
				}
			dragging = std::nullopt;
			}
		}

	void mouse_drag_view::on_mouse_scroll_wheel(const sf::Event::MouseWheelScrolled& event, sf::RenderTarget& render_target) noexcept
		{
		if (event.wheel == sf::Mouse::Wheel::Vertical)
			{
			if (event.delta != 0.f)
				{
				//const float new_zoom{std::max(get_zoom() + (get_zoom() * .1f * event.delta), .01f)};
				const float new_zoom{std::max(get_zoom() * (event.delta > 0 ? 1.1f : 0.9f), .01f)};

				const sf::Vector2i mouse{event.position};
				const sf::Vector2f pre_zoom_change_screen_as_wolrd{render_target.mapPixelToCoords(event.position)};

				set_zoom(new_zoom, render_target);

				const sf::Vector2i post_zoom_change_world_as_screen{render_target.mapCoordsToPixel(pre_zoom_change_screen_as_wolrd)};
				const sf::Vector2i delta_as_screen{post_zoom_change_world_as_screen - mouse};
				view.setCenter(view.getCenter() + (static_cast<sf::Vector2f>(delta_as_screen) / zoom));

				set_zoom(new_zoom, render_target);
				}
			}
		}

	void mouse_drag_view::process_events_not_requiring_capture(const sf::Event& event, sf::RenderTarget& render_target) noexcept
		{
		process_event<sf::Event::Resized>(event, [&](const sf::Event::Resized& event)
			{
			on_resize(event, render_target);
			});
		}
	bool mouse_drag_view::process_mouse_events(bool enable_left, const sf::Event& event, sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept
		{
		process_event<sf::Event::MouseWheelScrolled>(event, [&](const sf::Event::MouseWheelScrolled& event)
			{
			on_mouse_scroll_wheel(event, render_target);
			});
		process_event<sf::Event::MouseButtonPressed>(event, [&](const sf::Event::MouseButtonPressed& event)
			{
			on_mouse_pressed(enable_left, event, render_target);
			});
		process_event<sf::Event::MouseMoved>(event, [&](const sf::Event::MouseMoved& event)
			{
			on_mouse_moved(event, render_target);
			});
		process_event<sf::Event::MouseButtonReleased>(event, [&](const sf::Event::MouseButtonReleased& event)
			{
			on_mouse_released(enable_left, event, render_target, image_visualizer);
			});
		return is_mouse_captured();
		}

	bool mouse_drag_view::process_keyboard_events(const sf::Event& event, sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept
		{
		return process_event<sf::Event::KeyReleased>(event, [&](const sf::Event::KeyReleased& event)
			{
			return on_keyboard_pressed(event, render_target, image_visualizer);
			});
		}



	void mouse_drag_view::change_zoom_to_exact_pixel(const sf::Vector2f& target_size, sf::RenderTarget& render_target) noexcept
		{
		zoom = 1.f;

		centre_for(target_size, render_target);
		}

	void mouse_drag_view::change_zoom_to_fit_minor(const sf::Vector2f& target_size, sf::RenderTarget& render_target) noexcept
		{
		const auto available{static_cast<sf::Vector2f>(render_target.getSize())};

		const sf::Vector2f scale_to_fit{target_size.componentWiseDiv(available)};
		zoom = 1.f / std::min(scale_to_fit.x, scale_to_fit.y);

		centre_for(target_size, render_target);
		}

	void mouse_drag_view::change_zoom_to_fit_major(const sf::Vector2f& target_size, sf::RenderTarget& render_target) noexcept
		{
		const auto available{static_cast<sf::Vector2f>(render_target.getSize())};

		const sf::Vector2f scale_to_fit{target_size.componentWiseDiv(available)};
		zoom = 1.f / std::max(scale_to_fit.x, scale_to_fit.y);

		centre_for(target_size, render_target);
		}

	void mouse_drag_view::centre_for(const sf::Vector2f& target_size, sf::RenderTarget& render_target) noexcept
		{
		view.setCenter(target_size / 2.f);
		apply_zoom(render_target);
		}

	void mouse_drag_view::set_bounds(const sf::Vector2f& new_bounds, sf::RenderTarget& render_target) noexcept
		{
		bounds = new_bounds;
		enforce_bounds(render_target);
		}
	}