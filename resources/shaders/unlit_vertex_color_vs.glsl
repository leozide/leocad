LC_VERTEX_INPUT vec3 VertexPosition;
LC_VERTEX_INPUT vec4 VertexColor;
LC_VERTEX_OUTPUT vec4 PixelColor;

uniform mat4 WorldViewProjectionMatrix;

void main()
{
	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);
	PixelColor = VertexColor;
}
