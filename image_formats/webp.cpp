#include "webp.h"

#include <ranges>
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


	struct c_webp_image_wrapper
		{
		public:
			uint8_t* rgba_buffer{nullptr};
			int      width      {0      };
			int      height     {0      };

			operator bool() const noexcept { return rgba_buffer; }

			static c_webp_image_wrapper create(const std::span<uint8_t> encoded_buffer) noexcept
				{
				uint8_t* rgba_buffer{nullptr};
				int width;
				int height;

				rgba_buffer = WebPDecodeRGBA(encoded_buffer.data(), encoded_buffer.size(), std::addressof(width), std::addressof(height));

				return c_webp_image_wrapper{rgba_buffer, width, height};
				}

			~c_webp_image_wrapper()
				{
				if (rgba_buffer)
					{
					WebPFree(rgba_buffer);
					}
				}

			c_webp_image_wrapper           (const c_webp_image_wrapper&  copy) = delete;
			c_webp_image_wrapper& operator=(const c_webp_image_wrapper&  copy) = delete;
			c_webp_image_wrapper           (      c_webp_image_wrapper&& move) = delete;
			c_webp_image_wrapper& operator=(      c_webp_image_wrapper&& move) = delete;

		private:
			c_webp_image_wrapper(uint8_t* rgba_buffer, int width, int height) noexcept :
				rgba_buffer{rgba_buffer},
				width      {width      },
				height     {height     }
				{}
		};

	std::expected<sf::Image, std::string> load_webp(const std::filesystem::path& filepath) noexcept
		{
		std::ifstream file(filepath, std::ios::binary | std::ios::ate);
		if (!file.is_open()) 
			{
			return std::unexpected("Failed to open file: \"" + filepath.string() + "\"");
			}

		const std::streamsize size{file.tellg()};
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> encoded_buffer(size);
		if (!file.read(reinterpret_cast<char*>(encoded_buffer.data()), size))
			{
			return std::unexpected("Failed to read file: \"" + filepath.string() + "\"");
			}

		c_webp_image_wrapper c_webp_image_wrapper{c_webp_image_wrapper::create(encoded_buffer)};
		if (!c_webp_image_wrapper)
			{
			return std::unexpected("Failed to decode webp image: \"" + filepath.string() + "\"");
			}

		sf::Image sf_image;
		sf_image.resize(sf::Vector2u{static_cast<unsigned>(c_webp_image_wrapper.width), static_cast<unsigned>(c_webp_image_wrapper.height)}, c_webp_image_wrapper.rgba_buffer);
		return sf_image;
		}
	}