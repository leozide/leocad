LC_PIXEL_INPUT vec3 PixelNormal;
LC_PIXEL_OUTPUT

uniform mediump vec4 HighlightParams[4];
uniform samplerCube Texture;

void main()
{
	LC_SHADER_PRECISION float TexelAlpha = textureCube(Texture, PixelNormal).a;
	LC_SHADER_PRECISION float Distance = length(vec3(HighlightParams[0]) - PixelNormal);
	LC_SHADER_PRECISION float Highlight = step(Distance, HighlightParams[0].w);

	gl_FragColor = mix(mix(HighlightParams[2], HighlightParams[3], Highlight), HighlightParams[1], TexelAlpha);
}
