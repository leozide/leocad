LC_PIXEL_INPUT vec3 PixelNormal;
LC_PIXEL_INPUT vec3 PixelFaceCoord;
LC_PIXEL_OUTPUT

uniform mediump vec4 HighlightParams[4];
uniform mediump vec4 TextHaloColor;
uniform sampler2D Texture;

void main()
{
	LC_SHADER_PRECISION float Distance = length(vec3(HighlightParams[0]) - PixelNormal);
	LC_SHADER_PRECISION float Highlight = step(Distance, HighlightParams[0].w);
	LC_SHADER_PRECISION vec4 SurfaceColor = mix(HighlightParams[2], HighlightParams[3], Highlight);
	LC_SHADER_PRECISION vec2 FaceUV = PixelFaceCoord.xy;
	LC_SHADER_PRECISION float LabelMask = step(0.1, FaceUV.x) * step(FaceUV.x, 0.9) * step(0.3, FaceUV.y) * step(FaceUV.y, 0.7);
	LC_SHADER_PRECISION vec2 SampleUV = clamp(FaceUV, vec2(0.1, 0.3), vec2(0.9, 0.7));
	LC_SHADER_PRECISION vec2 AtlasUV = vec2(SampleUV.x, (PixelFaceCoord.z * 64.0 + SampleUV.y * 128.0 - 32.0) / 512.0);
	LC_SHADER_PRECISION float TextDistance = texture2D(Texture, AtlasUV).a;
	LC_SHADER_PRECISION float TextEdge = max(fwidth(TextDistance), 0.003);
	LC_SHADER_PRECISION float TextFill = smoothstep(0.5 - TextEdge, 0.5 + TextEdge, TextDistance);
	LC_SHADER_PRECISION float TextOutline = smoothstep(0.375 - TextEdge, 0.375 + TextEdge, TextDistance);
	LC_SHADER_PRECISION float Opacity = LabelMask * max(TextFill * HighlightParams[1].a, TextOutline * TextHaloColor.a);
	LC_SHADER_PRECISION vec3 InkColor = mix(TextHaloColor.rgb, HighlightParams[1].rgb, TextFill);
	gl_FragColor = vec4(mix(SurfaceColor.rgb, InkColor, Opacity), SurfaceColor.a);
}
