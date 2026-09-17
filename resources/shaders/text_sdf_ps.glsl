LC_PIXEL_INPUT vec2 PixelTexCoord;
LC_PIXEL_OUTPUT

uniform mediump vec4 MaterialColor;
uniform mediump vec4 TextHaloColor;
uniform sampler2D Texture;

void main()
{
	// The atlas stores a signed distance centered at 0.5. Derivatives keep the
	// edge smooth at every UI scale; the outer threshold is a fixed one-pixel halo.
	LC_SHADER_PRECISION float Distance = texture2D(Texture, PixelTexCoord).a;
	LC_SHADER_PRECISION float Edge = max(fwidth(Distance), 0.003);
	LC_SHADER_PRECISION float Fill = smoothstep(0.5 - Edge, 0.5 + Edge, Distance);
	LC_SHADER_PRECISION float Outline = smoothstep(0.375 - Edge, 0.375 + Edge, Distance);
	LC_SHADER_PRECISION float Luminance = dot(MaterialColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	LC_SHADER_PRECISION vec3 Color = mix(TextHaloColor.rgb, MaterialColor.rgb, Fill);

	gl_FragColor = vec4(Color, mix(TextHaloColor.a, MaterialColor.a, Fill) * Outline);
}
