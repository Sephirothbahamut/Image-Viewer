#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <mutex>
#include <thread>
#include <cmath>
#include <algorithm>
#include <stdexcept>

#include <SFML/Graphics.hpp>

#define NOMINMAX
#include <Windows.h>

#include "AnimatedGIF.h"

sf::Vector2f operator*(const sf::Vector2f& a, const sf::Vector2f& b) { return {a.x * b.x, a.y * b.y}; }
sf::Vector2f operator/(const sf::Vector2f& a, const sf::Vector2f& b) { return {a.x / b.x, a.y / b.y}; }
sf::Vector2f operator+(const sf::Vector2f& a, const sf::Vector2f& b) { return {a.x + b.x, a.y + b.y}; }
sf::Vector2f operator-(const sf::Vector2f& a, const sf::Vector2f& b) { return {a.x - b.x, a.y - b.y}; }

sf::Font font;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class Window;
Window* window_ptr{nullptr};
LONG_PTR sfml_window_proc;

class Window
	{
	public:
		Window(unsigned width, unsigned height, const std::string& title, std::vector<std::filesystem::path>& paths, size_t first = 0U) : 
			sf_window{sf::VideoMode{width, height}, title, sf::Style::Default},
			paths{paths},
			current_index{first}
			{
			sf_window.setKeyRepeatEnabled(true);
			texture.setSmooth(true);

			text_error.setPosition({0.f, 0.f});
			text_error.setOrigin((text_error.getLocalBounds().left + text_error.getLocalBounds().width) / 2, (text_error.getLocalBounds().top + text_error.getLocalBounds().height) / 2);
			text_error.setFillColor(sf::Color::White);


			HWND hwnd{sf_window.getSystemHandle()};
			sfml_window_proc = GetWindowLongPtr(hwnd, GWLP_WNDPROC);
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProc));

			window_ptr = this;

			load(current_index);
			}

		void handle_resize(unsigned width, unsigned height)
			{
			sf_window.setSize({width, height});

			adapt_view();
			draw();
			}

		void run()
			{
			draw();
			while (sf_window.isOpen())
				{
				sf::Event event;

				if (!is_gif)
					{
					if (sf_window.waitEvent(event))
						{
						switch (event.type)
							{
							case sf::Event::Closed:              sf_window.close();                         break;
							case sf::Event::MouseWheelScrolled:  on_mouse_wheel   (event.mouseWheelScroll); break;
							case sf::Event::MouseButtonPressed:  on_mouse_pressed (event.mouseButton);      break;
							case sf::Event::MouseMoved:          on_mouse_moved   (event.mouseMove);        break;
							case sf::Event::MouseButtonReleased: on_mouse_released(event.mouseButton);      break;
							case sf::Event::KeyPressed:          on_key_pressed   (event.key);              break;
							case sf::Event::KeyReleased:         on_key_released  (event.key);              break;
							}
						}
					}
				else
					{
					gif.update(sprite);
					draw();
					while (sf_window.pollEvent(event))
						{
						switch (event.type)
							{
							case sf::Event::Closed:              sf_window.close();                         break;
							case sf::Event::MouseWheelScrolled:  on_mouse_wheel   (event.mouseWheelScroll); break;
							case sf::Event::MouseButtonPressed:  on_mouse_pressed (event.mouseButton);      break;
							case sf::Event::MouseMoved:          on_mouse_moved   (event.mouseMove);        break;
							case sf::Event::MouseButtonReleased: on_mouse_released(event.mouseButton);      break;
							case sf::Event::KeyPressed:          on_key_pressed   (event.key);              break;
							case sf::Event::KeyReleased:         on_key_released  (event.key);              break;
							}
						}
					}
				}
			}

		void draw()
			{
			sf_window.clear();
			if (image_loaded) { sf_window.draw(sprite); }
			else { sf_window.draw(text_error); }
			sf_window.display();
			}
	private:

		sf::RenderWindow sf_window;
		sf::Texture texture;
		AnimatedGIF gif;
		sf::Sprite sprite;
		sf::Text text_error{"Unable to open file. The image might be corrupted or missing.", font, 24};
		bool image_loaded;
		bool is_gif;

		float zoom_factor{1};

		bool dragging{false};
		sf::Vector2f mouse_pos;

		std::vector<std::filesystem::path> paths;
		size_t current_index;

		void load(size_t index) { load(paths[index]); }
		void load(std::filesystem::path& path) 
			{
			sf_window.setTitle("Image Viewer - " + (path.has_filename() ? path.filename().string() : path.string()));
			if (path.extension().string() == ".gif")
				{
				image_loaded = gif.loadFromFile(path.string().c_str());
				if (image_loaded) 
					{
					gif.update(sprite);
					sprite.setTextureRect({{}, gif.getSize()});
					is_gif = true; 
					}
				}
			else
				{
				image_loaded = texture.loadFromFile(path.string());
				if (image_loaded) { sprite = sf::Sprite{texture}; is_gif = false; }
				}

			adapt_view();
			draw();
			}

		void on_key_pressed(sf::Event::KeyEvent e) {}
		void on_key_released(sf::Event::KeyEvent e)
			{
			switch (e.code)
				{
				case sf::Keyboard::Left:  current_index == 0 ? current_index = paths.size() - 1 : current_index--; load(current_index); break;
				case sf::Keyboard::Right: current_index == paths.size() - 1 ? current_index = 0 : current_index++; load(current_index); break;
				case sf::Keyboard::F5:                                                                             load(current_index); break;

				case sf::Keyboard::Num1:
				case sf::Keyboard::Num2:
				case sf::Keyboard::Num3:
				case sf::Keyboard::Num4:
				case sf::Keyboard::Num5:
				case sf::Keyboard::Num6:
				case sf::Keyboard::Num7:
				case sf::Keyboard::Num8:
				case sf::Keyboard::Num9: current_index = validate_index(e.code - sf::Keyboard::Num1); load(current_index); break;
				case sf::Keyboard::Num0: current_index = validate_index(10);                          load(current_index); break;
				case sf::Keyboard::Numpad1:
				case sf::Keyboard::Numpad2:
				case sf::Keyboard::Numpad3:
				case sf::Keyboard::Numpad4:
				case sf::Keyboard::Numpad5:
				case sf::Keyboard::Numpad6:
				case sf::Keyboard::Numpad7:
				case sf::Keyboard::Numpad8:
				case sf::Keyboard::Numpad9: current_index = validate_index(e.code - sf::Keyboard::Numpad1); load(current_index); break;
				case sf::Keyboard::Numpad0: current_index = validate_index(10);                             load(current_index); break;
				}
			
			}

		void on_mouse_wheel(sf::Event::MouseWheelScrollEvent& e)
			{
			if (dragging) { return; }

			if (e.delta < 0) { zoom_factor *= 1.1f; }
			else             { zoom_factor *= 0.9f; }

			auto new_size{recalc_view()};
			auto view{sf_window.getView()};
			view.setSize(new_size);
			sf_window.setView(view);

			draw();
			}
		void on_mouse_pressed(sf::Event::MouseButtonEvent e) { dragging = true; mouse_pos = get_mouse_pos(e.x, e.y); }
		void on_mouse_moved(sf::Event::MouseMoveEvent e)
			{
			if (!dragging) { return; }


			const sf::Vector2f new_pos = get_mouse_pos(e.x, e.y);

			const sf::Vector2f delta = mouse_pos - new_pos;

			auto view = sf_window.getView();
			view.setCenter(view.getCenter() + delta);
			sf_window.setView(view);

			mouse_pos = get_mouse_pos(e.x, e.y);

			draw();
			}
		void on_mouse_released(sf::Event::MouseButtonEvent e) { dragging = false; }


		// ======================================= UTILITY ======================================= 
		size_t validate_index(size_t i) { return i < paths.size() ? i : paths.size() - 1; }

		sf::Vector2f get_mouse_pos(int x, int y) { return sf_window.mapPixelToCoords({x, y}, sf_window.getView()); }

		void adapt_view()
			{

			sf::Vector2f size;

			if (image_loaded)
				{
				sf::Vector2f size = {sprite.getLocalBounds().width, sprite.getLocalBounds().height};

				zoom_factor = get_scale(size.x, size.y);

				auto new_size{recalc_view()};

				auto view{sf_window.getView()};
				view.setSize(new_size);
				view.setCenter(size / 2.f);
				sf_window.setView(view);
				}
			else
				{
				sf::Vector2f window_size{static_cast<float>(sf_window.getSize().x), static_cast<float>(sf_window.getSize().y)};
				auto view{sf_window.getView()};
				view.setCenter(0.f, 0.f);
				view.setSize(window_size);
				sf_window.setView(view);
				}
			}

		float get_scale(float width, float height)
			{
			sf::Vector2f window_size{static_cast<float>(sf_window.getSize().x), static_cast<float>(sf_window.getSize().y)};
			float h_scale{width  / window_size.x};
			float v_scale{height / window_size.y};

			return std::max(h_scale, v_scale);
			}

		sf::Vector2f recalc_view()
			{
			sf::Vector2f base_size{static_cast<float>(sf_window.getSize().x), static_cast<float>(sf_window.getSize().y)};
			return base_size * zoom_factor;
			}
	};


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
	Window& window = *window_ptr;

	switch (msg)
		{
		case WM_SIZE:
			{
			unsigned width  = static_cast<unsigned>LOWORD(lParam);  // Macro to get the low-order word.
			unsigned height = static_cast<unsigned>HIWORD(lParam);
			window.handle_resize(width, height);
			}
			break;
		}
	return CallWindowProc(reinterpret_cast<WNDPROC>(sfml_window_proc), hwnd, msg, wParam, lParam);
	}

#ifndef NDEBUG
#include <conio.h>
#endif
// Represents an error in a call to a Win32 API.
class win32_error : public std::runtime_error
	{
	public:
		win32_error(const char* msg, DWORD error)
			: std::runtime_error(msg)
			, _error(error)
			{}

		DWORD error() const
			{
			return _error;
			}

	private:
		DWORD _error;
	};


// Returns the full path of current EXE
std::wstring GetPathOfExe()
	{
	// Get filename with full path for current process EXE
	wchar_t filename[MAX_PATH];
	DWORD result = ::GetModuleFileName(
		nullptr,    // retrieve path of current process .EXE
		filename,
		_countof(filename)
	);
	if (result == 0)
		{
		// Error
		const DWORD error = ::GetLastError();
		throw win32_error("Error in getting module filename.",
			error);
		}

	return filename;
	}

void pause()
	{
#ifndef NDEBUG
	std::cout << "Press any key to continue . . . "; _getch();
#endif
	}

int main(int argc, char** argv)
	{
	std::vector<std::string> args; args.reserve(argc);
	for (int i = 0; i < argc; i++) { args.emplace_back(argv[i]); }

#ifndef NDEBUG
	for (const auto& arg : args) { std::cout << "\"" << arg << "\"\n"; }
	std::cout << "________________________________________" << std::endl;
	pause();
#endif

	if (true)
		{
		std::wstring this_path_str{GetPathOfExe()};
		std::filesystem::path this_path{this_path_str};
		this_path = this_path.parent_path();

		if (args.size() < 2)    { std::cerr << "Open with at least one image selected." << std::endl; pause(); return 0; }
		if (!font.loadFromFile(this_path.string() + "/consola.ttf")) { std::cerr << "Default font not found." << std::endl; pause(); return 0; }
		}

	std::vector<std::filesystem::path> paths; 
	size_t first_index{0};

	std::filesystem::path first{args[1]};
	if (!first.has_extension()) { std::cerr << "Expected an image. Supported formats: .bmp, .dds, .jpg, .png, .tga, .psd, .gif." << std::endl; pause(); return 0; }
	const auto extension = first.extension().string();
	if (extension != ".bmp" && extension != ".dds" && extension != ".jpg" && extension != ".png" && extension != ".tga" && extension != ".psd" && extension != ".gif")
		{
		std::cerr << "Expected an image. Supported formats: .bmp, .dds, .jpg, .png, .tga, .psd, .gif." << std::endl; pause(); return 0;
		}

	// If there are multiple images, show all of them. If there is only one and it has no parent path, show only that.
	// If there is only one image and it has a parent path, show all the images in the same folder

	if (args.size() > 2 || !first.has_parent_path())
		{
		paths.reserve(args.size() - 1);
		for (size_t i = 1; i < args.size(); i++) { paths.emplace_back(args[i]); }
		}
	else
		{
		std::filesystem::path root{first.parent_path()};

		for (const auto& entry : std::filesystem::directory_iterator(root))
			{
			const auto& path = entry.path();
			if (path.has_extension())
				{
				const auto extension = path.extension().string();
				if (extension == ".bmp" || extension == ".dds" || extension == ".jpg" || extension == ".png" || extension == ".tga" || extension == ".psd" || extension == ".gif")
					{
					paths.push_back(entry.path());
					}
				}
			}
		std::sort(paths.begin(), paths.end());
		for (size_t i = 0; i < paths.size(); i++) { if (paths[i] == first) { first_index = i; break; } }
		}



	Window window(800, 600, "Image viewer", paths, first_index);
	window.run();
	}