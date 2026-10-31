#pragma once

#include <expected>
#include <string>
#include <filesystem>

#include <SFML/Graphics/Image.hpp>

namespace image_formats
	{
	struct encoder
		{
		bool zero_transparent_pixels{false};
		bool lossless{true};
		//If lossless, determines compression quality (higher value is slower but produces smaller file)
		float compression{100};
		//If lossy, determines image quality (higher value produces bigger file, lower value ruins the image more)
		float quality{80};

		std::expected<void, std::string> save_webp(const sf::Image&, const std::filesystem::path& filepath) const noexcept;
		};

	std::expected<sf::Image, std::string> load_webp(const std::filesystem::path& filepath) noexcept;
	}