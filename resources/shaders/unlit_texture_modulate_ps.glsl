LC_PIXEL_INPUT vec2 PixelTexCoord;
LC_PIXEL_OUTPUT

uniform mediump vec4 MaterialColor;
uniform sampler2D Texture;

void main()
{
	LC_SHADER_PRECISION vec4 TexelColor = texture2D(Texture, PixelTexCoord);
	gl_FragColor = vec4(MaterialColor.rgb, TexelColor.a * MaterialColor.a);
}
