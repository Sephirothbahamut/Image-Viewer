#include "files_manager.h"

#include <unicode/coll.h>
#include <unicode/ustring.h>
#include <unicode/locid.h>

#include "global.h"

bool check_is_file_type_supported(const std::filesystem::path& path)
	{
	if (std::filesystem::is_directory(path) || !path.has_extension()) { return false; }

	const auto extension{path.extension()};

	//Nobody sane would put non-ascii characters as file extensions, right? Right? ;-;
	auto as_string{extension.string()};
	std::transform(as_string.begin(), as_string.end(), as_string.begin(), [](unsigned char c) { return std::tolower(c); });

	return as_string == ".bmp"
		|| as_string == ".dds"
		|| as_string == ".png"
		|| as_string == ".jpg"
		|| as_string == ".jpeg"
		|| as_string == ".tga"
		|| as_string == ".psd"
		|| as_string == ".webp";
	}

namespace files_manager
	{
	node::meta_type_t get_meta_type(const std::filesystem::path& path)
		{
		if (std::filesystem::is_directory(path))
			{
			return node::meta_type_t::folder;
			}
		else if (check_is_file_type_supported(path))
			{
			return node::meta_type_t::file_image;
			}
		else
			{
			return node::meta_type_t::file;
			}
		}









	node::node(const std::filesystem::path& relative_path, utils::observer_ptr<const node> parent_obs) :
		_relative_path         {relative_path            },
		_parent_obs            {parent_obs               },
		_meta_type             {get_meta_type(full_path())}
		{}

	//node::node(const node& copy) = delete;
	//node& node::operator=(const node& copy) = delete;


	node::node(node&& move) :
		_relative_path         {std::move(move._relative_path)},
		_parent_obs            {std::move(move._parent_obs   )},
		_children              {std::move(move._children     )},
		_meta_type             {std::move(move._meta_type    )}
		{
		for (auto& child : children())
			{
			child._parent_obs = this;
			}
		}

	node& node::operator=(node&& move)
		{
		_relative_path          = std::move(move._relative_path);
		_parent_obs             = std::move(move._parent_obs   );
		_children               = std::move(move._children     );
		_meta_type              = std::move(move._meta_type    );

		for (auto& child : children())
			{
			child._parent_obs = this;
			}

		return *this;
		}


	size_t node::index() const noexcept
		{
		if (!parent_obs()) { return std::numeric_limits<size_t>::max(); }
		for (size_t i{0}; i < parent_obs()->children().size(); i++)
			{
			const auto& child_obs{parent_obs()->_children[i].get()};
			if (child_obs == this) { return i; }
			}
		return std::numeric_limits<size_t>::max();
		}

	utils::observer_ptr  <const node> node::parent_obs   () const noexcept { return _parent_obs            ; }
	std::filesystem::path             node::relative_path() const noexcept { return _relative_path         ; }
	node::meta_type_t                 node::meta_type    () const noexcept { return _meta_type; }
	utils::observer_ptr  <const node> node::root_obs     () const noexcept
		{
		if (!parent_obs()) { return nullptr; }
		const auto& parent{*parent_obs()};
		if (parent.relative_path().empty()) { return this; }
		return parent.root_obs();
		}

	bool                              node::is_root() const noexcept { return root_obs() == this; }


	std::filesystem::path node::full_path() const noexcept
		{
		if (parent_obs() && !parent_obs()->_relative_path.empty())
			{
			return parent_obs()->full_path() / relative_path();
			}
		return relative_path();
		}

	std::vector<std::filesystem::path> node::partitioned_path() const noexcept
		{
		std::vector<std::filesystem::path> ret;
		if (parent_obs())
			{
			ret = parent_obs()->partitioned_path();
			}
		if (!relative_path().empty())
			{
			ret.emplace_back(relative_path());
			}
		return ret;
		}


	utils::observer_ptr<const node> node::next() const noexcept
		{
		if (!_children.empty())
			{
			const auto child_obs{_children[0].get()};
			return child_obs;
			}

		if (!parent_obs())
			{
			return nullptr;
			}

		const utils::observer_ptr<const node> next_sibling_obs{next_excluding_children()};
		if (next_sibling_obs) { return next_sibling_obs; }

		return parent_obs()->next_excluding_children();
		}

	utils::observer_ptr<const node> node::next_excluding_children() const noexcept
		{
		if (!parent_obs()) { return nullptr; }

		const auto& parent{*parent_obs()};
		const auto& parent_children{parent.children()};

		if (this == parent._children.rbegin()->get())
			{
			return nullptr;
			}
		return (parent._children.begin() + index() + 1)->get();
		}


	utils::observer_ptr<const node> node::next(const std::function<bool(const node& node)>& callback) const noexcept
		{
		utils::observer_ptr<const node> candidate_opt{next()};

		while (candidate_opt && !callback(*candidate_opt))
			{
			candidate_opt = candidate_opt->next();
			}

		return candidate_opt;
		}

	utils::observer_ptr<const node> node::next_excluding_children(const std::function<bool(const node& node)>& callback) const noexcept
		{
		utils::observer_ptr<const node> candidate_opt{next_excluding_children()};

		while (candidate_opt && !callback(*candidate_opt))
			{
			candidate_opt = candidate_opt->next();
			}

		return candidate_opt;
		}

	void node::create_children(bool do_create_descendants) noexcept
		{
		_children.clear();

		if (!std::filesystem::is_directory(full_path())) { return; }
		for (const auto& dir_entry : std::filesystem::directory_iterator{full_path()})
			{
			const auto& new_path{dir_entry.path()};
			auto& emplaced{_children.emplace_back(std::make_unique<node>(new_path.filename(), this))};
			}

		if (do_create_descendants)
			{
			for (auto& child : children())
				{
				child.create_children(do_create_descendants);
				}
			}
		}

	void node::sort() noexcept
		{
		UErrorCode status = U_ZERO_ERROR;

		// Create an ICU collator instance for the given locale
		icu::Locale locale{"EN", "UK"};
		std::unique_ptr<icu::Collator> collator(icu::Collator::createInstance(locale, status));

		if (U_FAILURE(status))
			{
			logger.err("Failed to create ICU Collator. Fallback to standard sorting.");
			std::sort(_children.begin(), _children.end(), [](const std::unique_ptr<node>& a, const std::unique_ptr<node>& b)
				{
				if (!a || !b) { return false; }
				return a->relative_path() < b->relative_path();
				});
			return;
			}

		collator->setAttribute(UCOL_NUMERIC_COLLATION, UCOL_ON, status);
		if (U_FAILURE(status))
			{
			logger.wrn("Could not set ICU collator to use natural numeric sortin.");
			}

		std::sort(_children.begin(), _children.end(), [&collator](const std::unique_ptr<node>& a, const std::unique_ptr<node>& b)
			{
			if (!a || !b) { return false; }

			const auto wa{a->relative_path().wstring()};
			const auto wb{b->relative_path().wstring()};

			const icu::UnicodeString strA(wa.data(), static_cast<int32_t>(wa.length()));
			const icu::UnicodeString strB(wb.data(), static_cast<int32_t>(wb.length()));

			UErrorCode status{U_ZERO_ERROR};
			const auto comparison{collator->compare(strA, strB, status)};

			if (U_FAILURE(status))
				{
				logger.err("Failed ICU string comparison.");
				return false;
				}

			return comparison == UCollationResult::UCOL_LESS;
			});

		for (auto& child : children())
			{
			child.sort();
			}
		}

	void node::purge() noexcept
		{
		_children.erase(std::remove_if(_children.begin(), _children.end(), [](const std::unique_ptr<node>& child_own)
			{
			const node& child{*child_own};
			return !std::filesystem::exists(child.full_path());
			}));

		for (auto& child : children())
			{
			child.purge();
			}
		}

	void node::for_each(std::function<void(const node&)> callback) const noexcept
		{
		callback(*this);
		for (const auto& node : children())
			{
			node.for_each(callback);
			}
		}

	void base::load(const std::span<const std::filesystem::path>& root_paths, bool check_subdirectories)
		{
		for (size_t i{0}; i < root_paths.size(); i++)
			{
			const auto& path{root_paths[i]};
			if (!std::filesystem::exists(path)) { continue; }

			const auto roots{forest_root.children()};
			const auto it{std::find_if(roots.begin(), roots.end(), [&](const node& child)
				{
				return child.relative_path() == path;
				})};
			if (it != roots.end()) { continue; }

			auto& emplaced{forest_root._children.emplace_back(std::make_unique<node>(path, std::addressof(forest_root)))};
			}

		for (auto& node : forest_root.children())
			{
			node.create_children(check_subdirectories);
			}
		}

	bool base::load(const std::filesystem::path& root, bool check_subdirectories)
		{
		if (!std::filesystem::exists(root)) { return false; }

		const auto roots{forest_root.children()};
		const auto it{std::find_if(roots.begin(), roots.end(), [&](const node& child)
			{
			return child.relative_path() == root;
			})};
		if (it != roots.end()) { return true; }

		auto& emplaced_own{forest_root._children.emplace_back(std::make_unique<node>(root, std::addressof(forest_root)))};
		auto& emplaced{*emplaced_own};

		emplaced.create_children(check_subdirectories);

		return true;
		}

	void base::remove_root(const node& to_remove) noexcept
		{
		forest_root._children.erase(std::remove_if(forest_root._children.begin(), forest_root._children.end(), [&](const std::unique_ptr<node>& node_own)
			{
			return node_own.get() == std::addressof(to_remove);
			}),
			forest_root._children.end());
		}

	void base::sort () noexcept { forest_root.sort (); }

	void base::purge() noexcept { forest_root.purge(); }

	void base::clear() noexcept { forest_root._children.clear(); }


	void base::load_subfolders() noexcept
		{
		for (auto& root : forest_root.children())
			{
			for (auto& child : root.children())
				{
				child.create_children(true);
				}
			}
		}

	void base::unload_subfolders() noexcept
		{
		for (auto& root : forest_root.children())
			{
			for (auto& child : root.children())
				{
				child._children.clear();
				}
			}
		}

	utils::observer_ptr<const node> base::first() const noexcept
		{
		return forest_root.next();
		}

	utils::observer_ptr<const node> base::first(const std::function<bool(const node& node)>& callback) const noexcept
		{
		return forest_root.next(callback);
		}


	void base::for_each(std::function<void(const node&)> callback) const noexcept
		{
		for (const auto& node : forest_root.children())
			{
			node.for_each(callback);
			}
		}
	
	bool selector::get_and_reset_changed() noexcept
		{
		const auto ret{changed};
		changed = false;
		return ret;
		}

	selector::selector(const std::function<bool(const node& candidate)>& filter, const with_selectors& manager) : filter{filter} 
		{
		selected_obs = manager.first(filter);
		}

	bool selector::test(const node& node) const noexcept
		{
		return filter(node);
		}

	utils::observer_ptr<const node> selector::set_or_null(const node& node) noexcept
		{
		changed = true;
		if (filter(node)) 
			{
			return selected_obs = std::addressof(node); 
			}
		return selected_obs = nullptr;
		}

	utils::observer_ptr<const node> selector::set_or_keep_current(const node& node) noexcept
		{
		if (filter(node))
			{
			changed = true;
			return selected_obs = std::addressof(node);
			}
		return selected_obs;
		}

	utils::observer_ptr<const node> selector::next(const with_selectors& manager) noexcept
		{
		changed = true;
		if (!selected_obs) { return selected_obs = manager.first(filter); }
		return selected_obs = selected_obs->next(filter);
		}

	utils::observer_ptr<const node> selector::first(const with_selectors& manager) noexcept
		{
		changed = true;
		return selected_obs = manager.first(filter);
		}


	selector::handle with_selectors::make_selector(const std::function<bool(const node& candidate)>& filter)
		{
		return selectors.make_unique(selectors.emplace(selector{filter, *this}));
		}

	void with_selectors::remove_root(const node& root_to_be_removed) noexcept
		{
		for (auto& selector : selectors)
			{
			auto& selected_obs{selector.selected_obs};
			if (!selector.selected_obs) { continue; }
			const auto& selected{*selector.selected_obs};

			if (const auto* root_obs{selector.selected_obs->root_obs()})
				{
				const auto& root{*root_obs};

				if (std::addressof(root) == std::addressof(root_to_be_removed))
					{
					selector.changed = true;
					selected_obs = root.next_excluding_children(selector.filter);
					}
				}
			}

		base::remove_root(root_to_be_removed);
		}

	void with_selectors::purge() noexcept
		{
		for (auto& selector : selectors)
			{
			auto& selected_obs{selector.selected_obs};
			if (!selector.selected_obs) { continue; }
			const auto& selected{*selector.selected_obs};

			if (!std::filesystem::exists(selected.full_path()))
				{
				selector.changed = true;
				selected_obs = selected.next_excluding_children([&](const node& candidate)
					{
					return (std::filesystem::exists(candidate.full_path()) && selector.filter(candidate));
					});
				}
			}

		base::purge();
		}

	utils::observer_ptr<const node> first_generation(const node& from) noexcept
		{
		if (from.parent_obs())
			{
			const auto& parent{*from.parent_obs()};
			const auto* grandparent_obs{parent.parent_obs()};
			if (grandparent_obs)
				{
				const auto& grandparent{*grandparent_obs};
				if (!grandparent.relative_path().empty())
					{
					return first_generation(parent);
					}
				}
			}
		return std::addressof(from);
		}

	void with_selectors::unload_subfolders() noexcept
		{
		for (size_t i{0}; i < selectors.size(); i++)
			{
			auto& selector{selectors[i]};
			auto& selected_obs{selector.selected_obs};
			if (!selector.selected_obs) { continue; }
			const auto& selected{*selector.selected_obs};
			selected_obs = first_generation(selected);
			selector.changed = true;
			}

		base::unload_subfolders();

		for (size_t i{0}; i < selectors.size(); i++)
			{
			auto& selector{selectors[i]};
			auto& selected_obs{selector.selected_obs};
			if (!selector.selected_obs) { continue; }
			const auto& selected{*selector.selected_obs};
			if (!selector.filter(selected))
				{
				selector.next(*this);
				}
			}
		}

	void with_selectors::clear() noexcept
		{
		for (auto& selector : selectors)
			{
			selector.selected_obs = nullptr;
			selector.changed = true;
			}
		base::clear();
		}
	}