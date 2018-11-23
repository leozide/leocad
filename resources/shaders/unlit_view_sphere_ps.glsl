LC_PIXEL_INPUT vec3 PixelNormal;
LC_PIXEL_OUTPUT

uniform mediump vec4 MaterialColor;
uniform mediump vec4 HighlightColor;
uniform samplerCube Texture;

void main()
{
	LC_SHADER_PRECISION float TexelAlpha = textureCube(Texture, PixelNormal).a;
	gl_FragColor = mix(MaterialColor, HighlightColor, TexelAlpha);
}
