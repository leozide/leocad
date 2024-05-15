LC_VERTEX_INPUT vec3 VertexPosition;

uniform mat4 WorldViewProjectionMatrix;

void main()
{
	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);
}
