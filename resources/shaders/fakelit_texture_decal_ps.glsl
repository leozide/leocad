LC_PIXEL_INPUT vec3 PixelPosition;
LC_PIXEL_INPUT vec3 PixelNormal;
LC_PIXEL_INPUT vec2 PixelTexCoord;
LC_PIXEL_OUTPUT

uniform mediump vec4 MaterialColor;
uniform mediump vec3 LightPosition;
uniform mediump vec3 EyePosition;
uniform sampler2D Texture;

void main()
{
	LC_PIXEL_FAKE_LIGHTING
	LC_SHADER_PRECISION vec4 TexelColor = texture2D(Texture, PixelTexCoord);
	LC_SHADER_PRECISION float Alpha = TexelColor.a + MaterialColor.a * (1.0 - TexelColor.a);
	LC_SHADER_PRECISION vec3 DiffuseColor = TexelColor.rgb * TexelColor.a + MaterialColor.rgb * MaterialColor.a * (1.0 - TexelColor.a);
	if (Alpha > 0.0)
		DiffuseColor /= Alpha;
	gl_FragColor = vec4(DiffuseColor * Diffuse + SpecularColor, Alpha);
}
