#include "webp.h"

#include <fstream>

#include <webp/encode.h>
#include <webp/decode.h>

namespace image_formats
	{
	//struct encoder
	//	{
	//	bool erase_transparent_pixels{false};
	//	bool lossless{true};
	//	//Compression quality for lossless
	//	int compression{100};
	//	//quality for lossy
	//	int quality{80};

	std::expected<void, std::string> encoder::save_webp(const sf::Image& image, const std::filesystem::path& filepath) const noexcept
		{
		WebPConfig config;
		WebPPicture pic;

		if (!WebPConfigInit(&config) || !WebPPictureInit(&pic)) {
			return std::unexpected{"Version mismatch or failed to initialize webp structs."};
			}

		config.lossless = lossless ? 1 : 0;
		config.exact    = zero_transparent_pixels ? 0 : 1;

		//If lossless, determines compression quality (higher value is slower but produces smaller file)
		//If lossy, determines image quality (higher value produces bigger file, lower value ruins the image more)
		config.quality  = lossless ? compression : quality; 

		pic.width  = image.getSize().x;
		pic.height = image.getSize().y;
		pic.use_argb = 1;         // Lossless mode requires the internal ARGB mesh representation

		// 4. Allocate internal webp memory buffers and import raw data
		// WebPPictureImportRGBA automatically handles the layout conversion into the working buffer.
		int stride = image.getSize().x * 4;
		if (!WebPPictureImportRGBA(&pic, image.getPixelsPtr(), stride))
			{
			WebPPictureFree(&pic);
			return std::unexpected{"Memory allocation failure or invalid RGBA pointer."};
			}

		WebPMemoryWriter writer;
		WebPMemoryWriterInit(&writer);
		pic.writer = WebPMemoryWrite;
		pic.custom_ptr = &writer;

		if (!WebPEncode(&config, &pic)) 
			{
			WebPPictureFree(&pic);
			WebPMemoryWriterClear(&writer);
			return std::unexpected{"Encoding failed."};
			}

		auto directory{filepath};
		directory.remove_filename();
		std::filesystem::create_directories(directory);
		std::ofstream out_file(filepath, std::ios::binary);
		if (out_file)
			{
			out_file.write(reinterpret_cast<const char*>(writer.mem), writer.size);
			}
		

		WebPPictureFree(&pic);
		WebPMemoryWriterClear(&writer);

		if (!out_file || !out_file.good())
			{
			return std::unexpected{"Failed to open output file: \"" + filepath.string() + "\""};
			}

		return {};
		};

	/*std::expected<sf::Image, std::string> load_webp(const std::filesystem::path& filepath) noexcept
		{

		}*/
	}