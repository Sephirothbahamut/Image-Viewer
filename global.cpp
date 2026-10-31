#include "global.h"

#include <utils/executable.h>

utils::logging::logger logger{utils::executable::get_directory() / "log.txt"};
sf::Texture default_texture;
sf::Font    default_font;