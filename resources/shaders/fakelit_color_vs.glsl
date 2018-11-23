LC_VERTEX_INPUT vec3 VertexPosition;
LC_VERTEX_INPUT vec3 VertexNormal;
LC_VERTEX_OUTPUT vec3 PixelPosition;
LC_VERTEX_OUTPUT vec3 PixelNormal;

uniform mat4 WorldViewProjectionMatrix;
uniform mat4 WorldMatrix;

void main()
{
	PixelPosition = (WorldMatrix * vec4(VertexPosition, 1.0)).xyz;
	PixelNormal = (WorldMatrix * vec4(VertexNormal, 0.0)).xyz;
	gl_Position = WorldViewProjectionMatrix * vec4(VertexPosition, 1.0);
}
