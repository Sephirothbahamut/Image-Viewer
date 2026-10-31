#include "viewport.h"

#include <utils/memory.h>
#include <utils/string.h>
#include <utils/compilation/OS.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui-SFML.h>

#include "../forkawesome.h"

int gui_filter_alphanum(ImGuiInputTextCallbackData* data) noexcept
	{//Return 0 is accepted, 1 is filtered out
	const bool lowercase {data->EventChar >= 'a' && data->EventChar <= 'z'};
	const bool uppercase {data->EventChar >= 'A' && data->EventChar <= 'Z'};
	const bool number    {data->EventChar >= '0' && data->EventChar <= '9'};
	const bool underscore{data->EventChar == '_'};
	const bool valid{lowercase || uppercase || number || underscore};
	return valid ? 0 : 1;
	}

int gui_filter_path(ImGuiInputTextCallbackData* data) noexcept
	{//Return 0 is accepted, 1 is filtered out
	if (gui_filter_alphanum(data) == 0) { return 0; }
	if (data->EventChar == '\\')
		{
		data->EventChar = '/';
		return 0;
		}
	const bool valid
		{
		data->EventChar == '%' ||
		data->EventChar == '/'
		};
	return valid ? 0 : 1;
	}

namespace gui
	{
	void cropper(image_panel::image_panel& image_panel, sf::RenderTarget& render_target) noexcept
		{
		auto& cropper{image_panel.cropper};
		auto& current{cropper.current};

		ImGui::PushID("cropper");
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);




		

		if (ImGui::Button("Create output file"))
			{
			cropper.create_output(image_panel.image_visualizer);
			}







		ImGui::SeparatorText("Current cropping data");

		ImGui::Text("Template name:");
		if (ImGui::InputText("##current_name", std::addressof(current.name), ImGuiInputTextFlags_CallbackCharFilter, &gui_filter_alphanum))
			{
			cropper.update_output_path(image_panel.image_visualizer);
			}

		ImGui::Text("Output path formula:");
		ImGui::BeginChild("Text horizontal scrolling", ImVec2{0.f, ImGui::GetTextLineHeightWithSpacing() * 3.f}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize(current.save_as.c_str()).x + 32.f));
		if (ImGui::InputText("##current_path", std::addressof(current.save_as), ImGuiInputTextFlags_CallbackCharFilter, &gui_filter_path))
			{
			cropper.update_output_path(image_panel.image_visualizer);
			}
		ImGui::EndChild();

		ImGui::Text("Resolved output path:");
		ImGui::BeginChild("Text horizontal scrolling 2", ImVec2{0.f, ImGui::GetTextLineHeightWithSpacing() * 3.f}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize(cropper.output_path.string().c_str()).x + 32.f));
		ImGui::Text(cropper.output_path.string().c_str());
		ImGui::EndChild();
		
		if (ImGui::CollapsingHeader("Syntax help"))
			{
			if(ImGui::SmallButton("%name%"                     )) { current.save_as += "%name%"                     ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%width%"                    )) { current.save_as += "%width%"                    ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%height%"                   )) { current.save_as += "%height%"                   ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_filename%"          )) { current.save_as += "%source_filename%"          ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_extension%"         )) { current.save_as += "%source_extension%"         ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_creation_year%"     )) { current.save_as += "%source_creation_year%"     ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_creation_month%"    )) { current.save_as += "%source_creation_month%"    ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_creation_day%"      )) { current.save_as += "%source_creation_day%"      ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_creation_hour%"     )) { current.save_as += "%source_creation_hour%"     ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_creation_minute%"   )) { current.save_as += "%source_creation_minute%"   ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_creation_second%"   )) { current.save_as += "%source_creation_second%"   ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_last_change_year%"  )) { current.save_as += "%source_last_change_year%"  ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_last_change_month%" )) { current.save_as += "%source_last_change_month%" ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_last_change_day%"   )) { current.save_as += "%source_last_change_day%"   ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_last_change_hour%"  )) { current.save_as += "%source_last_change_hour%"  ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_last_change_minute%")) { current.save_as += "%source_last_change_minute%"; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_last_change_second%")) { current.save_as += "%source_last_change_second%"; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_width%"             )) { current.save_as += "%source_width%"             ; cropper.update_output_path(image_panel.image_visualizer); }
			if(ImGui::SmallButton("%source_height%"            )) { current.save_as += "%source_height%"            ; cropper.update_output_path(image_panel.image_visualizer); }
			ImGui::TextWrapped("Note that creation date/time is unavailable in older Linux distributions.");
			}




		if (ImGui::CollapsingHeader("Rectangle"))
			{
			sf::Vector2i gui_ul{static_cast<sf::Vector2i>(current.ul)};
			sf::Vector2i gui_dr{static_cast<sf::Vector2i>(current.dr)};
			bool rect_changed{false};

			bool is_editing_mode_enabled{cropper.is_editing_mode_enabled()};
			if (ImGui::Checkbox("Drag on picture", std::addressof(is_editing_mode_enabled)))
				{
				cropper.set_editing_mode_enabled(is_editing_mode_enabled);
				}

			if (ImGui::DragInt("##up", std::addressof(gui_ul.y), 1.f, 0, gui_dr.y - 1)) { rect_changed = true; }

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x / 2.f);
			if (ImGui::DragInt("##ll", std::addressof(gui_ul.x), 1.f, 0, gui_dr.x - 1)) { rect_changed = true; }
			ImGui::SameLine();
			if (ImGui::DragInt("##rr", std::addressof(gui_dr.x), 1.f, gui_ul.x + 1, image_panel.image_visualizer.current_image_size().x)) { rect_changed = true; }
			ImGui::PopItemWidth();

			if (ImGui::DragInt("##dw", std::addressof(gui_dr.y), 1.f, gui_ul.y + 1, image_panel.image_visualizer.current_image_size().y)) { rect_changed = true; }

			if (rect_changed)
				{
				current.ul = static_cast<sf::Vector2u>(gui_ul);
				current.dr = static_cast<sf::Vector2u>(gui_dr);
				cropper.sfml_rectangle_update();
				cropper.update_output_path(image_panel.image_visualizer);
				}
			}
		if (ImGui::Button("Save as new template"))
			{
			cropper.templates.push_back(current);
			}

		if (ImGui::CollapsingHeader("Encoder settings"))
			{
			auto& encoder{current.encoder};

			ImGui::Checkbox("Lossless", std::addressof(encoder.lossless));

			if (encoder.lossless)
				{
				ImGui::DragFloat("Compression quality", std::addressof(encoder.compression), 1.f, 0.f, 100.f);
				ImGui::TextWrapped("Higher values produce smaller files but take longer to save.");
				}
			else
				{
				ImGui::DragFloat("Image quality", std::addressof(encoder.compression), 1.f, 0.f, 100.f);
				ImGui::TextWrapped("Lower values produce smaller files but cause more image quality loss.");
				}
			ImGui::Checkbox("Zero transparent pixels", std::addressof(encoder.zero_transparent_pixels));
			ImGui::TextWrapped("Will produce smaller files zeroing out RGB values of pixels with 0 alpha. Do not enable if there's data you care about in fully transparent pixels.");
			}

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Templates"))
			{
			ImGui::PushID("table");
			if (ImGui::BeginTable("table1", 3, ImGuiTableFlags_BordersOuter))
				{
				ImGui::TableSetupColumn("##cropper_column_a", ImGuiTableColumnFlags_None | ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("##cropper_column_b", ImGuiTableColumnFlags_None);

				ImGui::PushItemFlag(ImGuiItemFlags_AllowDuplicateId, true);

				for (size_t i = 0; i < cropper.templates.size(); i++)
					{
					ImGui::PushID(static_cast<int>(i));

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

					const auto& element{cropper.templates[i]};
					ImGui::Selectable((std::string(forkawesome::bars) + "##" + element.name).c_str());

					if (ImGui::IsItemActive() && !ImGui::IsItemHovered())
						{
						size_t n_next = i + (ImGui::GetMouseDragDelta(0).y < 0.0f ? -1 : 1);
						if (n_next >= 0 && n_next < cropper.templates.size())
							{
							std::swap(cropper.templates[i], cropper.templates[n_next]);
							ImGui::ResetMouseDragDelta();
							}
						}

					ImGui::TableSetColumnIndex(1);
					if (ImGui::Button(element.name.c_str()))
						{
						cropper.current = element;
						cropper.update_output_path(image_panel.image_visualizer);
						}

					ImGui::PopID();
					}

				ImGui::PopItemFlag();

				ImGui::EndTable();
				}

			if (ImGui::Button("Save templates")) { cropper.save_templates(); }
			if (ImGui::Button("Load templates")) { cropper.load_templates(); }

			ImGui::PopID();
			}

		if (ImGui::CollapsingHeader("Selection colour"))
			{
			auto cropper_colour{cropper.get_colour()};
			bool draw_pixel_grid_data_changed{false};
			std::array<float, 4> gui_colour
				{
				static_cast<float>(cropper_colour.r) / 255.f,
				static_cast<float>(cropper_colour.g) / 255.f,
				static_cast<float>(cropper_colour.b) / 255.f,
				static_cast<float>(cropper_colour.a) / 255.f
				};
			if (ImGui::ColorPicker4("##rect_colour", gui_colour.data(), ImGuiColorEditFlags_AlphaBar))
				{
				draw_pixel_grid_data_changed = true;
				cropper_colour =
					{
					static_cast<uint8_t>(gui_colour[0] * 255.f),
					static_cast<uint8_t>(gui_colour[1] * 255.f),
					static_cast<uint8_t>(gui_colour[2] * 255.f),
					static_cast<uint8_t>(gui_colour[3] * 255.f)
					};
				}
			if (draw_pixel_grid_data_changed) { cropper.set_colour(cropper_colour); }
			}


		ImGui::PopItemWidth();
		ImGui::PopID();
		}
	}