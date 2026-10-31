#include "cropper.h"

#include <format>

#include <utils/executable.h>
#include <utils/compilation/OS.h>

#include "../process_event.h"
#include "../global.h"

#include <glaze/glaze.hpp>


template <>
struct glz::meta<sf::Vector2u> {
	using T = sf::Vector2u;
	static constexpr auto value = glz::object(
		"x", &T::x,
		"y", &T::y
	);
	};

namespace image_panel
	{
	struct glz_metadata
		{
		std::array<unsigned, 4> rect;
		std::string name;
		};

	void cropper::load_templates() noexcept
		{
		std::string buffer;
		const glz::error_ctx error{glz::read_file_json(templates, (utils::executable::get_directory() / "templates.json").string(), buffer)};
		if (error) 
			{
			logger.err("Error loading cropper templates\n" + glz::format_error(error, buffer));
			}
		//std::vector<glz_metadata> json_data;
		//
		//std::string buffer;
		//const glz::error_ctx error{glz::read_file_json(json_data, (utils::executable::get_directory() / "templates.json").string(), buffer)};
		//if (error) 
		//	{
		//	logger.err("Error loading cropper templates\n" + glz::format_error(error, buffer));
		//	}
		//
		//templates.clear();
		//for (auto& tmp : json_data)
		//	{
		//	templates.push_back(crop_metadata{.ul{tmp.rect[0], tmp.rect[1]}, .dr{tmp.rect[2], tmp.rect[3]}, .name{std::move(tmp.name)}});
		//	}
		}

	void cropper::save_templates() noexcept
		{
		std::string buffer;
		const glz::error_ctx error{glz::write_file_json(templates, (utils::executable::get_directory() / "templates.json").string(), buffer)};
		if (error)
			{
			logger.err("Error loading cropper templates\n" + glz::format_error(error, buffer));
			}
		//std::vector<glz_metadata> json_data;
		//
		//for (const auto& tmp : templates)
		//	{
		//	json_data.push_back(glz_metadata{.rect{tmp.ul.x, tmp.ul.y, tmp.dr.x, tmp.dr.y}, .name{tmp.name}});
		//	}
		//
		//std::string buffer;
		//const glz::error_ctx error{glz::write_file_json(json_data, (utils::executable::get_directory() / "templates.json").string(), buffer)};
		//if (error)
		//	{
		//	logger.err("Error saving cropper templates\n" + glz::format_error(error, buffer));
		//	}
		}
	}