#pragma once

enum class lcBackgroundMode
{
	Default, // Preferences onscreen; transparent offscreen.
	Transparent,
	Opaque, // Configured solid or gradient background.
	CustomColor,
	CustomGradient
};

struct lcBackgroundOptions
{
	lcBackgroundMode Mode = lcBackgroundMode::Default;
	quint32 Color = 0; // Solid color or gradient top, in straight RGBA.
	quint32 GradientBottomColor = 0;
};
