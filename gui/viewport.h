#pragma once

#include <limits>

#include <utils/memory.h>

#include "../image_panel/image_panel.h"

namespace gui
	{
	void viewport(image_panel::image_panel& image_panel, sf::RenderTarget& render_target) noexcept;
	}