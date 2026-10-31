#include "viewport.h"

#include <utils/memory.h>
#include <utils/string.h>
#include <utils/compilation/OS.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui-SFML.h>

#include "../forkawesome.h"


namespace gui
	{
	void viewport(image_panel::image_panel& image_panel, sf::RenderTarget& render_target) noexcept
		{
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

		if (ImGui::CollapsingHeader("Zoom"))
			{
			if (ImGui::Button("Centre", ImVec2{-1.f, 0.f}))
				{
				image_panel.centre_for(render_target);
				}

			if (ImGui::Button("Exact pixel", ImVec2{-1.f, 0.f}))
				{
				image_panel.change_zoom_to_exact_pixel(render_target);
				}
			if (ImGui::Button("Fit major axis", ImVec2{-1.f, 0.f}))
				{
				image_panel.change_zoom_to_fit_major(render_target);
				}
			if (ImGui::Button("Fit minor axis", ImVec2{-1.f, 0.f}))
				{
				image_panel.change_zoom_to_fit_minor(render_target);
				}

			ImGui::SeparatorText("Current");
			float zoom_slider_value{image_panel.mouse_drag_view.get_zoom()};
			if (ImGui::SliderFloat("##current_zoom", std::addressof(zoom_slider_value), 0.01f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic))
				{
				image_panel.mouse_drag_view.set_zoom(zoom_slider_value, render_target);
				}

			ImGui::SeparatorText("Default zoom behaviour");
			if (ImGui::BeginCombo("##Default behaviour", image_panel::default_scale::as_string(image_panel.current_default_scale).data()))
				{
				for (size_t i{0}; i < 4; i++)
					{
					const bool is_selected{image_panel::default_scale::from_index(i) == image_panel.current_default_scale};
					if (ImGui::Selectable(image_panel::default_scale::as_string(i).data(), is_selected))
						{
						image_panel.current_default_scale = image_panel::default_scale::from_index(i);
						}
					if (is_selected)
						{
						ImGui::SetItemDefaultFocus();
						}
					}
				ImGui::EndCombo();
				}
			}
				
		if (ImGui::CollapsingHeader("Pixels grid"))
			{
			auto draw_pixel_grid_data{image_panel.get_draw_pixel_grid_data()};
			bool draw_pixel_grid_data_changed{false};
			std::array<float, 4> pixel_lines_colour
				{
				static_cast<float>(draw_pixel_grid_data.colour.r) / 255.f,
				static_cast<float>(draw_pixel_grid_data.colour.g) / 255.f,
				static_cast<float>(draw_pixel_grid_data.colour.b) / 255.f,
				static_cast<float>(draw_pixel_grid_data.colour.a) / 255.f
				};
			if (ImGui::ColorPicker4("##pixel_lines_colour", pixel_lines_colour.data(), ImGuiColorEditFlags_AlphaBar))
				{
				draw_pixel_grid_data_changed = true;
				draw_pixel_grid_data.colour =
					{
					static_cast<uint8_t>(pixel_lines_colour[0] * 255.f),
					static_cast<uint8_t>(pixel_lines_colour[1] * 255.f),
					static_cast<uint8_t>(pixel_lines_colour[2] * 255.f),
					static_cast<uint8_t>(pixel_lines_colour[3] * 255.f)
					};
				}
			ImGui::Text("Show pixel lines at or below zoom level:");
			if (ImGui::SliderFloat("##pixel_lines_max_zoom", std::addressof(draw_pixel_grid_data.minimum_zoom), 5.f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp))
				{
				draw_pixel_grid_data_changed = true;
				}
			if (draw_pixel_grid_data_changed) { image_panel.set_draw_pixel_grid_data(draw_pixel_grid_data); }
			}

		if (ImGui::CollapsingHeader("Texture"))
			{
			bool smoothing{image_panel.image_visualizer.get_smooth()};
			if (ImGui::Checkbox("Image smoothing", std::addressof(smoothing)))
				{
				image_panel.image_visualizer.set_smooth(smoothing);
				}
			}
		ImGui::PopItemWidth();
		}
	}