#include <chrono>
#include <vector>
#include <filesystem>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <utils/memory.h>
#include <utils/string.h>
#include <utils/executable.h>
#include <utils/logging/logger.h>
#include <utils/console/initializer.h>
#include <utils/containers/handled_container.h>

#include <imgui.h>
#include <imgui-SFML.h>

#include "forkawesome.h"

#include "image_panel/image_panel.h"
#include "files_manager.h"

#include "gui/tree_view.h"
#include "gui/viewport.h"
#include "gui/cropper.h"

#include "process_event.h"

//std::vector<std::filesystem::path> paths
//	{
//	"C:/Users/Sephi/Downloads/2026-01-26/"
//	};

template <typename char_t>
std::vector<std::filesystem::path> strings_to_paths(const std::span<const std::basic_string_view<char_t>>& strings)
	{
	std::vector<std::filesystem::path> ret;
	for (const auto& string : strings)
		{
		const std::filesystem::path as_path{string};
		if (std::filesystem::exists(as_path))
			{
			ret.emplace_back(as_path);
			}
		}
	return ret;
	}

bool file_manager_selector_filter(const ::files_manager::node& node)
	{
	return check_is_file_type_supported(node.full_path());
	}

class main_class
	{
	public:
	bool use_imgui{true};

	sf::Clock clock;
	sf::RenderWindow window;

	ImFont* font_default;
	ImFont* font_awesome;

	files_manager::with_selectors files_manager;
	files_manager::selector::handle selector_handle;

	gui::tree_view tree_view; //Must be after files_manager

	image_panel::image_panel image_panel;

	public:
		
		void imgui_setup()
			{
			if (!ImGui::SFML::Init(window)) { throw std::runtime_error{"ImGui SFML error, could not update font texture."}; }


			auto& io{ImGui::GetIO()};
			io.IniFilename = NULL;

			io.Fonts->Clear();
			io.Fonts->AddFontDefault();

			float baseFontSize = 13.0f; // 13.0f is the size of the default font. Change to the font size you use.
			float iconFontSize = baseFontSize; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

			// merge in icons from Font Awesome
			static const ImWchar icons_ranges[] = {::forkawesome::ICON_MIN_FA, ::forkawesome::ICON_MAX_16_FA, 0};
			ImFontConfig icons_config;
			icons_config.MergeMode = true;
			icons_config.PixelSnapH = true;
			icons_config.GlyphMinAdvanceX = iconFontSize;
			io.Fonts->AddFontFromFileTTF((utils::executable::get_directory() / ::forkawesome::FONT_ICON_FILE_NAME_FK).string().c_str(), iconFontSize, &icons_config, icons_ranges);
			if (!ImGui::SFML::UpdateFontTexture()) { throw std::runtime_error{"ImGui SFML error, could not update font texture."}; }
			}

		main_class() : 
			window{sf::VideoMode{{800, 600}}, "Image Viewer + Cutter"},
			image_panel{window},
			selector_handle{files_manager.make_selector(&file_manager_selector_filter)}
			{
			window.setKeyRepeatEnabled(true);
			imgui_setup();
			}

		void load(std::span<const std::filesystem::path> paths)
			{
			files_manager.load(paths, tree_view.get_gui_load_subfolders());
			selector_handle->first(files_manager);
			if (selector_handle->selected_obs)
				{
				image_panel.try_load(selector_handle->selected_obs->full_path(), window);
				}
			}

		void imgui_step()
			{
			ImGui::SFML::Update(window, clock.restart());

			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			//ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowSizeConstraints({32.f, viewport->WorkSize.y}, {viewport->WorkSize.x - 32.f, viewport->WorkSize.y});
			//ImGui::SetNextWindowViewport(viewport->ID); // in docking branch
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			
			ImGuiWindowFlags flags = /*ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |*/ ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

			ImGui::Begin("##Main", 0, flags);
			if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_FittingPolicyScroll))
				{
				if (ImGui::BeginTabItem("Files browser"))
					{
					tree_view.draw(files_manager, selector_handle);

					ImGui::EndTabItem();
					}

				if (ImGui::BeginTabItem("Viewport"))
					{
					gui::viewport(image_panel, window);
					ImGui::EndTabItem();
					}

				if (ImGui::BeginTabItem("Cropping"))
					{
					gui::cropper(image_panel, window);
					ImGui::EndTabItem();
					}

				if (ImGui::BeginTabItem("Controls"))
					{
					ImGui::SeparatorText("Viewport");
					ImGui::TextWrapped("Hold your mouse's left or right click to drag.");
					ImGui::TextWrapped("Use scrolling wheel to zoom in/out. Zoom is centered on current mouse position.");
					ImGui::TextWrapped("Use arrow keys to move the viewport.");
					ImGui::TextWrapped("Use +/- to zoom in and out.");
					ImGui::TextWrapped("Press [space] to fit the image on its major axis. Hold [CTRL] to fit the image on its minor axis. Hold [ALT] to make it pixel exact. ");
					
					ImGui::SeparatorText("Files tree");
					ImGui::TextWrapped("Use [left]/[right] arrow keys or [forward]/[backward] mouse buttons to go to the next/previous image.");

					ImGui::SeparatorText("Cropper");
					ImGui::TextWrapped("Hold [ALT] and use arrow keys to extend the cropping area in the arrow direction. Hold [CTRL] and [ALT] to shrink it.");
					ImGui::TextWrapped("Press enter to generate the output image.");
					ImGui::TextWrapped("Press numbers from 0 to 9 to select the corresponding cropping template.");

					ImGui::EndTabItem();
					}
				ImGui::EndTabBar();
				}
			ImGui::End();

			ImGui::PopStyleVar(2);
			}

		void step()
			{
			if (use_imgui)
				{
				imgui_step();
				}

			if (selector_handle->get_and_reset_changed() && selector_handle->selected_obs)
				{
				if (image_panel.try_load(selector_handle->selected_obs->full_path(), window))
					{
					window.setTitle("Image Viewer + Cutter | " + selector_handle->selected_obs->relative_path().string());
					}
				}
			}

		void draw()
			{
			window.clear();

			window.pushGLStates();
			image_panel.draw(window);
			window.popGLStates();

			if (use_imgui) { ImGui::SFML::Render(window); }

			window.display();
			}

		void loop()
			{
			while (window.isOpen())
				{
				if (const auto event_opt{window.waitEvent()})
					{
					const auto& event{*event_opt};

					if (event.is<sf::Event::Closed>())
						{
						window.close();
						}

					image_panel.process_events_not_requiring_capture(event, window);

					if (use_imgui) 
						{
						ImGui::SFML::ProcessEvent(window, event); 
						}

					if (!use_imgui || !ImGui::GetIO().WantCaptureMouse   ) { image_panel.process_mouse_events   (event, window, files_manager, selector_handle); } 
					if (!use_imgui || !ImGui::GetIO().WantCaptureKeyboard)
						{
						image_panel.process_keyboard_events(event, window, files_manager, selector_handle);

						process_event<sf::Event::KeyPressed>(event, [&](const sf::Event::KeyPressed& event)
							{
							if (event.code == sf::Keyboard::Key::Space)
								{
								use_imgui = !use_imgui;
								}
							});
						}
					}

				step();
				draw();
				}
			}

	};

void apply_zoom(float zoom, sf::RenderTarget& render_target) noexcept
	{
	const auto viewport_size{static_cast<sf::Vector2f>(render_target.getSize())};
	auto view{render_target.getView()};
	view.setSize(viewport_size * zoom);
	render_target.setView(view);
	}

int main(int argc, const char* argv[])
	{
	utils::console::initializer utils_console_initializer;

	static std::vector<std::string_view> args(argv, argv + argc);

	std::vector<std::filesystem::path> paths
		{
		/**
		::paths
		/*/
		strings_to_paths<char>(args)
		//*/
		};
	
	if (!default_font.openFromFile("Arial.ttf"))
		{
		logger.err("SFML: Failed to load font file.");
		}

	main_class mc;

	mc.load(paths);

	mc.loop();
	return 0;
	}
