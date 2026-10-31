#include "cropper.h"

#include <fstream>
#include <expected>

#include <utils/memory.h>

#include "../global.h"
#include "../image_formats/webp.h"

#include <webp/encode.h>

namespace image_panel
	{
	void cropper::create_output(const image_visualizer& image_visualizer) const noexcept
		{
		const std::filesystem::path target_path{[&]()
			{
			auto tmp{image_visualizer.current_path()};
			tmp.remove_filename();
			return tmp / output_path;
			}()};

		const std::filesystem::path source_path{image_visualizer.current_path()};
		if (source_path.empty()) 
			{
			logger.wrn("Cannot create output, no image file selected."); 
			return; 
			}
		const auto _{logger.section("Cropping\n\tFrom: \"" + source_path.string() + "\"\n\tTo:   \"" + target_path.string() + "\".")};

		const sf::Image& source_image{image_visualizer.get_current_image()};

		const auto source_resolution{source_image.getSize()};
		if (source_resolution.x == 0 || source_resolution.y == 0)
			{
			logger.err("Invalid source image.");
			return;
			}

		const sf::Vector2u source_dr
			{
			std::min(current.dr.x, source_resolution.x),
			std::min(current.dr.y, source_resolution.y)
			};

		const sf::Vector2u source_ul
			{
			std::min(current.ul.x, current.dr.x - 1u),
			std::min(current.ul.y, current.dr.y - 1u)
			};

		const sf::Vector2u target_resolution{source_dr - source_ul};
		sf::Image target_image{target_resolution, sf::Color::Transparent};

		for (unsigned int y{0}; y < target_resolution.y; y++)
			{
			for (unsigned int x{0}; x < target_resolution.x; x++)
				{
				target_image.setPixel({x, y}, source_image.getPixel(source_ul + sf::Vector2u{x, y}));
				}
			}

		if (const auto result{current.encoder.save_webp(target_image, target_path)})
			{
			std::cout << "\a" << std::flush;
			}
		else
			{
			logger.err("Libwebp saving failed " + result.error());
			}
		}
	}