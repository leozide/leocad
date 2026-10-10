LC_PIXEL_INPUT vec2 PixelTexCoord;
LC_PIXEL_OUTPUT

uniform sampler2D Texture;
uniform sampler2D VisibleTexture;
uniform mediump vec4 OutlineParams[3]; // Pixel size / outline radius / hidden opacity, selected color, focused color.

LC_SHADER_PRECISION vec2 OutlineMaskCoverage(LC_SHADER_PRECISION vec2 Position)
{
	if (Position.x < 0.0 || Position.y < 0.0 || Position.x > 1.0 || Position.y > 1.0)
		return vec2(0.0);

	return texture2D(Texture, Position).rg;
}

void main()
{
	LC_SHADER_PRECISION vec2 Coverage = OutlineMaskCoverage(PixelTexCoord);
	LC_SHADER_PRECISION vec2 Expanded = Coverage;
	LC_SHADER_PRECISION vec2 PixelSize = OutlineParams[0].xy;
	LC_SHADER_PRECISION float Radius = min(OutlineParams[0].z, 8.0);

	// A disk of physical pixels keeps thin silhouettes connected on HiDPI
	// displays. Constant loop bounds also work with desktop GLSL 1.10.
	for (int Y = -8; Y <= 8; Y++)
	{
		if (abs(float(Y)) > Radius)
			continue;

		for (int X = -8; X <= 8; X++)
		{
			LC_SHADER_PRECISION vec2 Offset = vec2(float(X), float(Y));

			if (dot(Offset, Offset) <= Radius * Radius)
				Expanded = max(Expanded, OutlineMaskCoverage(PixelTexCoord + PixelSize * Offset));
		}
	}

	LC_SHADER_PRECISION float Border = max(Expanded.r - Coverage.r, 0.0);
	LC_SHADER_PRECISION float Visible = texture2D(VisibleTexture, PixelTexCoord).r;
	LC_SHADER_PRECISION float Hidden = Coverage.r * (1.0 - Visible);
	LC_SHADER_PRECISION vec4 FillColor = mix(OutlineParams[1], OutlineParams[2], Coverage.g);
	LC_SHADER_PRECISION vec4 OutlineColor = mix(OutlineParams[1], OutlineParams[2], Expanded.g);

	gl_FragColor = vec4(mix(FillColor.rgb, OutlineColor.rgb, Border), max(Hidden * OutlineParams[0].w, Border));
}
