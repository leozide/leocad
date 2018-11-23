LC_PIXEL_INPUT vec3 PixelPosition;
LC_PIXEL_INPUT vec3 PixelNormal;
LC_PIXEL_OUTPUT

uniform mediump vec4 MaterialColor;
uniform mediump vec3 LightPosition;
uniform mediump vec3 EyePosition;

void main()
{
	LC_PIXEL_FAKE_LIGHTING
	LC_SHADER_PRECISION vec3 DiffuseColor = MaterialColor.rgb * Diffuse;
	gl_FragColor = vec4(DiffuseColor + SpecularColor, MaterialColor.a);
}
