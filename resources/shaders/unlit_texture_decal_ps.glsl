LC_PIXEL_INPUT vec2 PixelTexCoord;
LC_PIXEL_OUTPUT

uniform mediump vec4 MaterialColor;
uniform sampler2D Texture;

void main()
{
	LC_SHADER_PRECISION vec4 TexelColor = texture2D(Texture, PixelTexCoord);
	LC_SHADER_PRECISION float Alpha = TexelColor.a + MaterialColor.a * (1.0 - TexelColor.a);
	LC_SHADER_PRECISION vec3 Color = TexelColor.rgb * TexelColor.a + MaterialColor.rgb * MaterialColor.a * (1.0 - TexelColor.a);
	if (Alpha > 0.0)
		Color /= Alpha;
	gl_FragColor = vec4(Color, Alpha);
}
