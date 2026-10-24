LC_PIXEL_INPUT vec2 PixelTexCoord;
LC_PIXEL_OUTPUT

uniform sampler2D Texture;
uniform sampler2D VisibleTexture;
uniform mediump vec4 OutlineParams[4]; // Pixel size / outline radius / hidden opacity, selected color, focused color, highlight color.

LC_SHADER_PRECISION vec3 OutlineMaskCoverage(LC_SHADER_PRECISION vec2 Position)
{
	if (Position.x < 0.0 || Position.y < 0.0 || Position.x > 1.0 || Position.y > 1.0)
		return vec3(0.0);

	return texture2D(Texture, Position).rgb;
}

void main()
{
	LC_SHADER_PRECISION vec3 Coverage = OutlineMaskCoverage(PixelTexCoord);
	LC_SHADER_PRECISION vec3 Expanded = Coverage;
	LC_SHADER_PRECISION vec2 ExpandedOpacity = Coverage.rb;
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
			{
				LC_SHADER_PRECISION vec3 SampleCoverage = OutlineMaskCoverage(PixelTexCoord + PixelSize * Offset);
				LC_SHADER_PRECISION float Opacity = 1.0 - smoothstep(0.0, Radius, max(length(Offset) - 1.0, 0.0));

				Expanded = max(Expanded, SampleCoverage);
				ExpandedOpacity = max(ExpandedOpacity, SampleCoverage.rb * Opacity);
			}
		}
	}

	LC_SHADER_PRECISION float Border = max(Expanded.r - Coverage.r, 0.0);
	LC_SHADER_PRECISION float BorderOpacity = max(ExpandedOpacity.x - Coverage.r, 0.0);
	LC_SHADER_PRECISION float HighlightOpacity = max(ExpandedOpacity.y - Coverage.b, 0.0) * (1.0 - Coverage.r) * OutlineParams[3].a;
	LC_SHADER_PRECISION float Visible = texture2D(VisibleTexture, PixelTexCoord).r;
	LC_SHADER_PRECISION float Hidden = Coverage.r * (1.0 - Visible);
	LC_SHADER_PRECISION vec4 FillColor = mix(OutlineParams[1], OutlineParams[2], Coverage.g);
	LC_SHADER_PRECISION vec4 OutlineColor = mix(OutlineParams[1], OutlineParams[2], Expanded.g);
	LC_SHADER_PRECISION vec3 SelectionColor = mix(FillColor.rgb, OutlineColor.rgb, Border);
	LC_SHADER_PRECISION float SelectionOpacity = max(Hidden * OutlineParams[0].w, BorderOpacity);
	LC_SHADER_PRECISION float Opacity = SelectionOpacity + HighlightOpacity * (1.0 - SelectionOpacity);
	LC_SHADER_PRECISION vec3 Color = SelectionColor * SelectionOpacity + OutlineParams[3].rgb * HighlightOpacity * (1.0 - SelectionOpacity);

	gl_FragColor = vec4(Color / max(Opacity, 0.0001), Opacity);
}
