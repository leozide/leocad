LC_PIXEL_INPUT vec3 PixelNormal;
LC_PIXEL_OUTPUT

uniform mediump vec4 HighlightParams[2];
uniform samplerCube Texture;

const mediump vec4 TextColor = vec4(0.0, 0.0, 0.0, 1.0);
const mediump vec4 BackgroundColor = vec4(1.0, 1.0, 1.0, 1.0);
const mediump vec4 HighlightColor = vec4(1.0, 0.0, 0.0, 1.0);

void main()
{
	LC_SHADER_PRECISION float TexelAlpha = textureCube(Texture, PixelNormal).a;

	if (PixelNormal.x > HighlightParams[0].x && PixelNormal.y > HighlightParams[0].y && PixelNormal.z > HighlightParams[0].z &&
		PixelNormal.x < HighlightParams[1].x && PixelNormal.y < HighlightParams[1].y && PixelNormal.z < HighlightParams[1].z)
		gl_FragColor = mix(HighlightColor, TextColor, TexelAlpha);
	else
		gl_FragColor = mix(BackgroundColor, TextColor, TexelAlpha);
}
