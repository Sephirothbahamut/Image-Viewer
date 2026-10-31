#pragma once

#include <limits>

#include <SFML/Window.hpp>

#include <utils/memory.h>

#include "../files_manager.h"

namespace gui
	{
	class tree_view
		{
		private:
			utils::observer_ptr<const files_manager::node> gui_to_delete{nullptr};
			size_t gui_drawing_line_index{0};

			bool gui_load_subfolders{false};
			std::string gui_add_element_path;

			void draw_root_delete(const files_manager::node& node) noexcept;
			template <bool root>
			void draw_node(const files_manager::node& node, files_manager::selector::handle& selector_handle) noexcept;

			void draw_tree_nodes(files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle) noexcept;
			void draw_add_line  (files_manager::with_selectors& files_manager, const sf::Window& parent_window) noexcept;
			void draw_tree_block(files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle, const sf::Window& parent_window) noexcept;

		public:
			bool get_gui_load_subfolders() const noexcept;

			void draw(files_manager::with_selectors& files_manager, files_manager::selector::handle& selector_handle, const sf::Window& parent_window) noexcept;
		};
	}