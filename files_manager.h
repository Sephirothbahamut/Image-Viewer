#pragma once

#include <ranges>
#include <vector>
#include <filesystem>
#include <functional>

#include <utils/memory.h>
#include <utils/string.h>
#include <utils/containers/handled_container.h>

bool check_is_file_type_supported(const std::filesystem::path& path);

namespace files_manager
	{
	class base;

	
	class node
		{
		friend class files_manager::base;
		public:
			enum class meta_type_t { unknown, folder, file, file_image };

			node(const std::filesystem::path& relative_path, utils::observer_ptr<const node> parent_obs);
			node(const node& copy) = delete;
			node& operator=(const node& copy) = delete;
			node(node&& move);
			node& operator=(node&& move);

		private:
			std::filesystem::path _relative_path{""};
			utils::observer_ptr<const node> _parent_obs{nullptr};

			std::vector<std::unique_ptr<node>> _children;
			meta_type_t _meta_type{meta_type_t::unknown};

			auto children() noexcept
				{
				return _children | std::views::transform([](std::unique_ptr<node>& node) -> files_manager::node&
					{
					return *node;
					});
				}

		public:
			auto children() const noexcept
				{
				return _children | std::views::transform([](const std::unique_ptr<node>& node) -> const files_manager::node& 
					{
					return *node;
					});
				}
			size_t                          index        () const noexcept;
			utils::observer_ptr<const node> parent_obs   () const noexcept;
			utils::observer_ptr<const node> root_obs     () const noexcept;
			std::filesystem::path           relative_path() const noexcept;
			meta_type_t                     meta_type    () const noexcept;

			std::filesystem::path full_path() const noexcept;

			std::vector<std::filesystem::path> partitioned_path() const noexcept;

		public:
			/// <summary> Default constructor for the forest "root" that contains all the forest roots. </summary>
			node() {};

		public:
			utils::observer_ptr<const node> next() const noexcept;
			utils::observer_ptr<const node> next(const std::function<bool(const files_manager::node& node)>& callback) const noexcept;
			utils::observer_ptr<const node> next_excluding_children() const noexcept;
			utils::observer_ptr<const node> next_excluding_children(const std::function<bool(const files_manager::node& node)>& callback) const noexcept;

		private:

		private:
			void create_children(bool do_create_descendants = false) noexcept;

			void sort() noexcept;

			void purge() noexcept;

			void for_each(std::function<void(const node&)> callback) const noexcept;
		};



	class base
		{
		private:
			node forest_root{"", nullptr};

		public:
			void load(const std::span<const std::filesystem::path>& root_paths, bool check_subdirectories);
			bool load(const std::filesystem::path& root, bool check_subdirectories);
			void remove_root(const node& root) noexcept;
			void sort() noexcept;

			/// <summary> Removes all nodes which path doesn't exist in the filesystem. </summary>
			void purge() noexcept;

			void clear() noexcept;

			void load_subfolders() noexcept;
			void unload_subfolders() noexcept;

			utils::observer_ptr<const files_manager::node> first() const noexcept;
			utils::observer_ptr<const files_manager::node> first(const std::function<bool(const files_manager::node& node)>& callback) const noexcept;

			auto roots() const noexcept { return forest_root.children(); }

			void for_each(std::function<void(const node&)> callback) const noexcept;
		};

	class with_selectors;

	class selector
		{
		friend class files_manager::with_selectors;
		private:
			std::function<bool(const node& candidate)> filter{[](const node&) { return true; }};

			selector(const std::function<bool(const node& candidate)>& filter, const with_selectors& manager);

			bool changed{true};

		public:

			bool get_and_reset_changed() noexcept;

			using handle = utils::containers::handled_container<selector>::handle_unique;

			utils::observer_ptr<const node> selected_obs{nullptr};

			bool test(const node& node) const noexcept;
			utils::observer_ptr<const node> set_or_null(const node& node) noexcept;
			utils::observer_ptr<const node> set_or_keep_current(const node& node) noexcept;
			utils::observer_ptr<const node> next (const with_selectors& manager) noexcept;
			utils::observer_ptr<const node> first(const with_selectors& manager) noexcept;
		};

	class with_selectors : private base
		{
		public:
			using base::load;
			using base::sort;

			using base::load_subfolders;

			using base::first;
			using base::roots;

			using base::for_each;

		private:
			using selectors_t = utils::containers::handled_container<selector>;

		private:
			utils::containers::handled_container<selector> selectors;

		public:
			selector::handle make_selector(const std::function<bool(const node& candidate)>& filter);

			void remove_root(const node& root_to_be_removed) noexcept;

			void purge() noexcept;

			void unload_subfolders() noexcept;

			void clear() noexcept;
		};
	}