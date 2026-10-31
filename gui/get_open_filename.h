#pragma once

#include <vector>
#include <optional>
#include <filesystem>

#include <SFML/Window.hpp>

#include <utils/memory.h>

#include "../files_manager.h"

std::optional<std::vector<std::filesystem::path>> user_selected_files      (const sf::Window& owner) noexcept;
std::optional<std::vector<std::filesystem::path>> user_selected_directories(const sf::Window& owner) noexcept;