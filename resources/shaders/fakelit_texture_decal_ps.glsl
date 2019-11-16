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
	LC_SHADER_PRECISION vec3 DiffuseColor = mix(MaterialColor.rgb, TexelColor.rgb, TexelColor.a);
	gl_FragColor = vec4(DiffuseColor * Diffuse + SpecularColor, max(TexelColor.a, MaterialColor.a));
}
