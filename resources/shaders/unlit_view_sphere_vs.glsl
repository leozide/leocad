LC_VERTEX_INPUT vec3 VertexPosition;
LC_VERTEX_OUTPUT vec3 PixelNormal;

uniform mat4 WorldViewProjectionMatrix;

void main()
{
	PixelNormal = normalize(VertexPosition);
	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);
}
