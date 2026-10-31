#include "cropper.h"

#include <utils/compilation/OS.h>
#include <format>

#include "../process_event.h"
#include "../global.h"

#ifdef utils_compilation_os_windows
#define NOMINMAX
#include <windows.h>
#elif utils_compilation_os_linux
#include <sys/stat.h>
#include <fcntl.h>
#endif

inline constexpr const bool is_creation_time_supported
	{
	#if defined(utils_compilation_os_windows) || (defined(utils_compilation_os_linux) && defined(STATX_BTIME))
	true
	#else
	false
	#endif
	};

struct times
	{
	std::string year   {"NA"};
	std::string month  {"NA"};
	std::string day    {"NA"};
	std::string hours  {"NA"};
	std::string minutes{"NA"};
	std::string seconds{"NA"};

	inline static times from_system_timepoint(const std::chrono::system_clock::time_point& time_point) noexcept
		{
		const auto local_time{std::chrono::current_zone()->to_local(time_point)};
    
		const auto day_point{std::chrono::floor<std::chrono::days>(local_time)};
		const std::chrono::year_month_day ymd{day_point};
		const std::chrono::hh_mm_ss hms{local_time - day_point};

		return times
			{
			.year    {std::format("{}"   , ymd.year   ())},
			.month   {std::format("{:%m}", ymd.month  ())},
			.day     {std::format("{}"   , ymd.day    ())},
			.hours   {std::format("{:%H}", hms.hours  ())},
			.minutes {std::format("{:%M}", hms.minutes())},
			.seconds {std::format("{:%S}", hms.seconds())}
			};
		}
	};

// Platform-independent helper to extract file creation/birth time
std::chrono::system_clock::time_point get_creation_time(const std::filesystem::path& file_path) noexcept
	{
	if constexpr (!is_creation_time_supported)
		{
		return {};
		}

	#ifdef utils_compilation_os_windows
		{
		// Windows API strategy
		WIN32_FILE_ATTRIBUTE_DATA file_info{};
		if (!GetFileAttributesExW(file_path.c_str(), GetFileExInfoStandard, &file_info)) 
			{
			throw std::filesystem::filesystem_error("Failed to read file attributes", file_path, std::make_error_code(std::errc::no_such_file_or_directory));
			}

		// Combine low and high bits into a 64-bit Windows FILETIME intervals (100-nanoseconds since 1601)
		const uint64_t intervals{(static_cast<uint64_t>(file_info.ftCreationTime.dwHighDateTime) << 32) | file_info.ftCreationTime.dwLowDateTime};

		// Convert Windows Epoch (Jan 1, 1601) intervals to standard Unix Epoch duration
		const std::chrono::duration<int64_t, std::nano> unix_nanos{(intervals - 116444736000000000ULL) * 100};
		return std::chrono::system_clock::time_point{std::chrono::duration_cast<std::chrono::system_clock::duration>(unix_nanos)};
		}
	#elifdef utils_compilation_os_linux
		{
		// Linux statx strategy (modern kernels/filesystems supporting birth time)
		struct statx stx {};
		if (statx(AT_FDCWD, file_path.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BTIME, &stx) != 0) {
			throw std::filesystem::filesystem_error("statx failed", file_path, std::make_error_code(static_cast<std::errc>(errno)));
			}

		if (!(stx.stx_mask & STATX_BTIME)) {
			throw std::filesystem::filesystem_error("Birth time not supported by filesystem", file_path, std::make_error_code(std::errc::not_supported));
			}

		const std::chrono::seconds secs{stx.stx_btime.tv_sec};
		const std::chrono::nanoseconds nsecs{stx.stx_btime.tv_nsec};
		return std::chrono::system_clock::time_point{std::chrono::duration_cast<std::chrono::system_clock::duration>(secs + nsecs)};
		}
	#endif
	}

times get_creation_times(const std::filesystem::path& file_path) noexcept
	{
	const auto system_time{get_creation_time(file_path)};
	return times::from_system_timepoint(system_time);
	}

times get_last_write_times(const std::filesystem::path& path) noexcept
	{
	const auto file_time{std::filesystem::last_write_time(path)};
	const auto system_time{std::chrono::clock_cast<std::chrono::system_clock>(file_time)};
	return times::from_system_timepoint(system_time);
	}

const std::string extract_filename(const std::filesystem::path& path) noexcept
	{
	const auto filename_with_extension{path.filename()};
	if (!filename_with_extension.has_extension()) { return filename_with_extension.string(); }
	const auto extension_size{filename_with_extension.extension().string().size()};
	std::string ret{filename_with_extension.string()};
	ret = ret.substr(0, ret.size() - extension_size);
	return ret;
	}

std::string evaluate_variable(const image_panel::cropper& cropper, const image_panel::image_visualizer& image_visualizer, const std::string_view& variable_name, const times& last_change_times, const times& creation_times) noexcept
	{
	if (variable_name == "name"                     ) { return cropper.current.name; }
	if (variable_name == "width"                    ) { return std::to_string(cropper.current.dr.x - cropper.current.ul.x); }
	if (variable_name == "height"                   ) { return std::to_string(cropper.current.dr.y - cropper.current.ul.y); }
	if (variable_name == "source_filename"          ) { return extract_filename(image_visualizer.current_path()); }
	if (variable_name == "source_extension"         ) { return image_visualizer.current_path().extension().string(); }
	if (variable_name == "source_creation_year"     ) { return creation_times   .year   ; }
	if (variable_name == "source_creation_month"    ) { return creation_times   .month  ; }
	if (variable_name == "source_creation_day"      ) { return creation_times   .day    ; }
	if (variable_name == "source_creation_hour"     ) { return creation_times   .hours  ; }
	if (variable_name == "source_creation_minute"   ) { return creation_times   .minutes; }
	if (variable_name == "source_creation_second"   ) { return creation_times   .seconds; }
	if (variable_name == "source_last_change_year"  ) { return last_change_times.year   ; }
	if (variable_name == "source_last_change_month" ) { return last_change_times.month  ; }
	if (variable_name == "source_last_change_day"   ) { return last_change_times.day    ; }
	if (variable_name == "source_last_change_hour"  ) { return last_change_times.hours  ; }
	if (variable_name == "source_last_change_minute") { return last_change_times.minutes; }
	if (variable_name == "source_last_change_second") { return last_change_times.seconds; }
	if (variable_name == "source_width"             ) { return std::to_string(image_visualizer.current_image_size().x); }
	if (variable_name == "source_height"            ) { return std::to_string(image_visualizer.current_image_size().y); }
	return "[invalid parameter]";
	}

std::string evaluate_template_string(const image_panel::cropper& cropper, const image_panel::image_visualizer& image_visualizer, const std::string_view& input) noexcept
	{
	const times last_change_times{get_last_write_times(image_visualizer.current_path())};
	const times creation_times   {get_creation_times  (image_visualizer.current_path())};

	std::string output{};
	output.reserve(input.size());
		
	std::string_view range{input.begin(), input.end()};

	while (!range.empty())
		{
		const size_t variable_begin{range.find_first_of('%')};
		if (variable_begin == std::string::npos)
			{
			output.append(range);
			break;
			}
			
		output.append(range.substr(0, variable_begin));
		range.remove_prefix(variable_begin + 1);

		const size_t variable_end{range.find_first_of('%')};
		if (variable_end == std::string::npos)
			{
			//TODO error
			break;
			}
		const std::string_view variable_name{range.substr(0, variable_end)};

		output.append(evaluate_variable(cropper, image_visualizer, variable_name, last_change_times, creation_times));

		range.remove_prefix(variable_end + 1);
		}

	return output;
	}




namespace image_panel
	{
	void cropper::sfml_rectangle_update() noexcept
		{
		const sf::Vector2f ul_f{static_cast<sf::Vector2f>(current.ul)};
		const sf::Vector2f dr_f{static_cast<sf::Vector2f>(current.dr)};

		sfml_vertices_array[0].position =  ul_f;
		sfml_vertices_array[1].position = {dr_f.x, ul_f.y};
		sfml_vertices_array[2].position =  dr_f;
		sfml_vertices_array[3].position = {ul_f.x, dr_f.y};
		sfml_vertices_array[4].position =  ul_f;
		}

	bool cropper::is_dragging() const noexcept { return dragging.has_value(); }
	bool cropper::is_editing_mode_enabled() const noexcept { return editing_mode_enabled; }
	void cropper::set_editing_mode_enabled(bool enabled) noexcept
		{
		dragging = std::nullopt;
		editing_mode_enabled = enabled;
		}

	sf::Color cropper::get_colour() const noexcept 
		{
		return sfml_vertices_array[0].color;
		}
	void cropper::set_colour(sf::Color new_value) noexcept 
		{
		for (auto& vertex : sfml_vertices_array)
			{
			vertex.color = new_value;
			}
		}

	void cropper::clamp_rect(const sf::Vector2u& texture_size) noexcept
		{
		current.dr =
			{
			std::min(current.dr.x, texture_size.x),
			std::min(current.dr.y, texture_size.y)
			};

		current.ul = 
			{
			std::min(current.ul.x, current.dr.x - 1u),
			std::min(current.ul.y, current.dr.y - 1u)
			};
		sfml_rectangle_update();
		}

	void cropper::update_output_path(const image_visualizer& image_visualizer) noexcept
		{
		output_path = evaluate_template_string(*this, image_visualizer, current.save_as) + ".webp";
		}

	bool cropper::on_keyboard_pressed(const sf::Event::KeyReleased& event, const sf::RenderTarget& render_target, const image_visualizer& image_visualizer) noexcept
		{
		if (event.code == sf::Keyboard::Key::Enter)
			{
			create_output(image_visualizer);
			return true;
			}
		
		if (event.alt && (event.code >= sf::Keyboard::Key::Left) && (event.code <= sf::Keyboard::Key::Down))
			{
			if(event.control)
				{
				switch (event.code)
					{
					case sf::Keyboard::Key::Left : if (current.ul.x < (current.dr.x - 1)) { current.ul.x -= 1; } break;
					case sf::Keyboard::Key::Up   : if (current.ul.y < (current.dr.y - 1)) { current.ul.y -= 1; } break;
					case sf::Keyboard::Key::Right: if (current.dr.x > (current.ul.x + 1)) { current.dr.x += 1; } break;
					case sf::Keyboard::Key::Down : if (current.dr.y > (current.ul.y + 1)) { current.dr.y += 1; } break;
					}
				}
			else
				{
				switch (event.code)
					{
					case sf::Keyboard::Key::Left : if (current.ul.x > 0                                            ) { current.ul.x -= 1; } break;
					case sf::Keyboard::Key::Up   : if (current.ul.y > 0                                            ) { current.ul.y -= 1; } break;
					case sf::Keyboard::Key::Right: if (current.dr.x < (image_visualizer.current_image_size().x - 1)) { current.dr.x += 1; } break;
					case sf::Keyboard::Key::Down : if (current.dr.y < (image_visualizer.current_image_size().y - 1)) { current.dr.y += 1; } break;
					}
				}

			sfml_rectangle_update();
			return true;
			}

		const bool is_num{(event.code >= sf::Keyboard::Key::Num0   ) && (event.code <= sf::Keyboard::Key::Num9   )};
		const bool is_pad{(event.code >= sf::Keyboard::Key::Numpad0) && (event.code <= sf::Keyboard::Key::Numpad9)};

		if (!is_num && !is_pad) { return false; }

		const size_t index{is_num ? static_cast<size_t>(event.code) - static_cast<size_t>(sf::Keyboard::Key::Num0) : static_cast<size_t>(event.code) - static_cast<size_t>(sf::Keyboard::Key::Numpad0)};
		if (index <= templates.size())
			{
			current = templates[index];
			sfml_rectangle_update();
			}
		return true;
		}

	void cropper::on_mouse_pressed(const sf::Event::MouseButtonPressed& event, const sf::RenderTarget& render_target, const sf::Vector2u& texture_size) noexcept
		{
		if (event.button != sf::Mouse::Button::Left) { return; }

		const sf::Vector2f position_in_world{render_target.mapPixelToCoords(event.position)};
		const sf::Vector2f position_on_image
			{
			std::floor(std::clamp(position_in_world.x, 0.f, static_cast<float>(texture_size.x))),
			std::floor(std::clamp(position_in_world.y, 0.f, static_cast<float>(texture_size.y)))
			};

		dragging = current.ul = current.dr = static_cast<sf::Vector2u>(position_on_image);

		for (auto& vertex : sfml_vertices_array)
			{
			vertex.position = position_on_image;
			}
		}

	void cropper::on_mouse_moved(const sf::Event::MouseMoved& event, const sf::RenderTarget& render_target, const sf::Vector2u& texture_size) noexcept
		{
		if (is_dragging())
			{
			const sf::Vector2f position_in_world_f{render_target.mapPixelToCoords(event.position)};
			const sf::Vector2f position_in_world_f_positive{std::max(position_in_world_f.x, 0.f), std::max(position_in_world_f.y, 0.f)};
			const sf::Vector2u position_in_world_pixels{static_cast<sf::Vector2u>(position_in_world_f_positive)};
			const sf::Vector2u position_on_image
				{
				std::min(position_in_world_pixels.x, texture_size.x),
				std::min(position_in_world_pixels.y, texture_size.y)
				};

			current.ul = {std::min({dragging->x, position_on_image.x    , texture_size.x - 1u}), std::min({dragging->y, position_on_image.y    , texture_size.y - 1u})};
			current.dr = {std::max({dragging->x, position_on_image.x + 1,                  1u}), std::max({dragging->y, position_on_image.y + 1,                  1u})};

			current.dr.x = std::min(current.dr.x, texture_size.x);
			current.dr.y = std::min(current.dr.y, texture_size.y);
			sfml_rectangle_update();
			}
		}

	void cropper::on_mouse_released(const sf::Event::MouseButtonReleased& event) noexcept
		{
		if (event.button != sf::Mouse::Button::Left) { return; }

		dragging = std::nullopt;
		}

	bool cropper::process_mouse_events(const sf::Event& event, sf::RenderTarget& render_target, const sf::Vector2u& texture_size) noexcept
		{
		if (is_editing_mode_enabled())
			{
			process_event<sf::Event::MouseButtonPressed>(event, [&](const sf::Event::MouseButtonPressed& event)
				{
				on_mouse_pressed(event, render_target, texture_size);
				});
			process_event<sf::Event::MouseMoved>(event, [&](const sf::Event::MouseMoved& event)
				{
				on_mouse_moved(event, render_target, texture_size);
				});
			process_event<sf::Event::MouseButtonReleased>(event, [&](const sf::Event::MouseButtonReleased& event)
				{
				on_mouse_released(event);
				});
			return is_dragging();
			}
		return false;
		}

	bool cropper::process_keyboard_events(const sf::Event& event, sf::RenderTarget& render_target, const sf::Vector2u& texture_size, const image_visualizer& image_visualizer) noexcept
		{
		if (is_editing_mode_enabled())
			{
			return process_event<sf::Event::KeyReleased>(event, [&](const sf::Event::KeyReleased& event)
				{
				return on_keyboard_pressed(event, render_target, image_visualizer);
				});
			}
		return false;
		}

	void cropper::draw(sf::RenderTarget& render_target)
		{
		render_target.draw(sfml_vertices_array);
		}
	}