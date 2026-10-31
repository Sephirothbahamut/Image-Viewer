#include "tree_view.h"

#include <utils/memory.h>
#include <utils/string.h>
#include <utils/compilation/OS.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui-SFML.h>

#include "../global.h"
#include "../forkawesome.h"

#include "get_open_filename.h"


namespace gui
	{	
	struct table_row_background_colour_ret_t
		{
		float r, g, b;
		table_row_background_colour_ret_t operator*(float value) const noexcept
			{
			return table_row_background_colour_ret_t{r * value, g * value, b * value};
			}
		ImVec4 to_imvec4(float alpha) const noexcept 
			{
			return ImVec4{r, g, b, alpha};
			};
		};

	constexpr const table_row_background_colour_ret_t table_row_background_colour(const ::files_manager::node& node)
		{
		switch (node.meta_type())
			{
			case ::files_manager::node::meta_type_t::unknown   : return table_row_background_colour_ret_t{1.f, 0.f, 0.f};
			case ::files_manager::node::meta_type_t::folder    : return table_row_background_colour_ret_t{1.f, 1.f, 1.f};
			case ::files_manager::node::meta_type_t::file      : return table_row_background_colour_ret_t{1.f, .5f, 0.f};
			case ::files_manager::node::meta_type_t::file_image: return table_row_background_colour_ret_t{.5f, 1.f, .5f};
			}
		std::unreachable();
		}

	constexpr const std::string_view icon_string(const ::files_manager::node& node)
		{
		switch (node.meta_type())
			{
			case ::files_manager::node::meta_type_t::unknown   : return ::forkawesome::question     ;
			case ::files_manager::node::meta_type_t::folder    : return ::forkawesome::folder_open_o;
			case ::files_manager::node::meta_type_t::file      : return ::forkawesome::file_o       ;
			case ::files_manager::node::meta_type_t::file_image: return ::forkawesome::file_image_o ;
			}
		std::unreachable();
		}

	void tree_view::draw_root_delete(const ::files_manager::node& node) noexcept
		{
		ImGui::TableSetColumnIndex(1);
		if (ImGui::SmallButton(::forkawesome::window_close_o.data()))
			{
			gui_to_delete = std::addressof(node);
			}
		}

	template <bool root>
	void tree_view::draw_node(const ::files_manager::node& node, files_manager::selector::handle& selector_handle) noexcept
		{
		const size_t my_index{gui_drawing_line_index};
		const bool i_am_selected{std::addressof(node) == selector_handle->selected_obs};
		gui_drawing_line_index++;

		//Kind-of optimization to no draw text in non visible rows. Does NOT use ImGui's clipping as it requires lots more work (tree flattening etcc) even though that's more performant.
		const size_t scroll_first_visible_index{static_cast<size_t>(ImGui::GetScrollY() / ImGui::GetTextLineHeightWithSpacing())};
		const size_t scroll_last_visible_index {scroll_first_visible_index + static_cast<size_t>(ImGui::GetCurrentWindow()->Size.y / ImGui::GetTextLineHeightWithSpacing())};
		
		const bool before_visible_end {my_index <= scroll_last_visible_index };
		const bool after_visible_begin{my_index >= scroll_first_visible_index};
		const bool draw_me{before_visible_end && after_visible_begin};

		if (draw_me)
			{
			const ImGuiTreeNodeFlags flags
				{
				ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth |
				(i_am_selected ? ImGuiTreeNodeFlags_Selected : 0) |
				(root ? 0 : ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns) |
				(node.children().empty() ? ImGuiTreeNodeFlags_Leaf : 0)
				};

			const std::string string{std::string{icon_string(node)} + ' ' + utils::string::cast<char>(node.relative_path().operator std::filesystem::path::string_type())};

			ImGui::TableNextRow();

			const ImU32 row_bg_color{ImGui::GetColorU32((table_row_background_colour(node) * ((my_index % 2) ? 0.18f : 0.22f)).to_imvec4(.6f))};
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_bg_color);

			ImGui::TableSetColumnIndex(0);
			if (ImGui::TreeNodeEx(std::addressof(node), flags, string.c_str()))
				{
				if (ImGui::IsItemClicked() || (ImGui::IsItemFocused() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space))))
					{
					selector_handle->set_or_keep_current(node);
					}
				
				if constexpr (root) { draw_root_delete(node); }

				for (const auto& child : node.children())
					{
					draw_node<false>(child, selector_handle);
					}

				ImGui::TreePop();
				}
			else if constexpr (root) { draw_root_delete(node); }
			}
		else
			{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			if (ImGui::TreeNodeEx(std::addressof(node), ImGuiTreeNodeFlags_None, ""))
				{
				for (const auto& child : node.children())
					{
					draw_node<false>(child, selector_handle);
					}

				ImGui::TreePop();
				}
			}
		}

	void tree_view::draw_tree_nodes(files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept
		{
		const auto roots{files_manager.roots()};

		gui_drawing_line_index = 0;
		for (size_t i{0}; i < roots.size(); i++)
			{
			ImGui::PushID(static_cast<int>(i));

			const auto& root{roots[i]};
			draw_node<true>(root, selector_handle);

			ImGui::PopID();
			}
		}

	void tree_view::draw_add_line(files_manager::with_selectors& files_manager, const sf::Window& parent_window) noexcept
		{
		if (ImGui::BeginTable("table_add", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuterH))
			{
			ImGui::TableSetupColumn("##add_column_a", ImGuiTableColumnFlags_None | ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##add_column_b", ImGuiTableColumnFlags_None);

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
			
			ImGui::InputText("##Add root path", std::addressof(gui_add_element_path) );
			ImGui::PopItemWidth();

			ImGui::TableSetColumnIndex(1);
			if (ImGui::Button((std::string(::forkawesome::plus_square_o) + "##btn add files").c_str()))
				{
				files_manager.load(gui_add_element_path, gui_load_subfolders);
				}

			ImGui::EndTable();
			}

		const auto button_width{ImGui::GetContentRegionAvail().x / 2.f};
		if (ImGui::Button("Add file", ImVec2{button_width, 0.f}))
			{
			if (const auto selected_opt{user_selected_files(parent_window)})
				{
				const auto& selected{*selected_opt};
				files_manager.load(std::span{selected.begin(), selected.end()}, gui_load_subfolders);
				}
			}
		ImGui::SameLine();
		if (ImGui::Button("Add directory", ImVec2{button_width, 0.f}))
			{
			if (const auto selected_opt{user_selected_directories(parent_window)})
				{
				const auto& selected{*selected_opt};
				files_manager.load(std::span{selected.begin(), selected.end()}, gui_load_subfolders);
				}
			}
		}

	void tree_view::draw_tree_block(files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle, const sf::Window& parent_window) noexcept
		{
		ImGui::BeginChild("Tree block", ImVec2(0, 260), ImGuiChildFlags_ResizeY, ImGuiWindowFlags_None);
	
		ImGuiListClipper clipper;
		const ImGuiTableFlags flags{ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit};
		const auto sz{ImGui::GetCurrentWindow()->Size};
		if (ImGui::BeginTable("table_scrolly", 2, flags, sz))
			{
			ImGui::TableSetupColumn("##column_a", ImGuiTableColumnFlags_None | ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##column_b", ImGuiTableColumnFlags_None);

			draw_tree_nodes(files_manager, selector_handle);
			ImGui::EndTable();
			}

		ImGui::EndChild();

		if (gui_to_delete)
			{
			files_manager.remove_root(*gui_to_delete);
			gui_to_delete = nullptr;
			}

		draw_add_line(files_manager, parent_window);
		}

	bool tree_view::get_gui_load_subfolders() const noexcept
		{
		return gui_load_subfolders;
		}

	void tree_view::draw(files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle, const sf::Window& parent_window) noexcept
		{
		ImGui::PushID(123);

		draw_tree_block(files_manager, selector_handle, parent_window);

		if (ImGui::Checkbox("Load subfolders", std::addressof(gui_load_subfolders)))
			{
			if (gui_load_subfolders)
				{
				files_manager.load_subfolders();
				}
			else
				{
				files_manager.unload_subfolders();
				}
			}

		ImGui::PopID();

		if (ImGui::Button("Sort alphabetically"))
			{
			files_manager.sort();
			}
		}
	}