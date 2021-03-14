LC_VERTEX_INPUT vec3 VertexPosition1;
LC_VERTEX_INPUT vec3 VertexPosition2;
LC_VERTEX_INPUT vec3 VertexPosition3;
LC_VERTEX_INPUT vec3 VertexPosition4;

uniform mat4 WorldViewProjectionMatrix;

void main()
{
	vec4 p1 = WorldViewProjectionMatrix * vec4(VertexPosition1, 1.0);
	vec4 p2 = WorldViewProjectionMatrix * vec4(VertexPosition2, 1.0);
	vec4 p3 = WorldViewProjectionMatrix * vec4(VertexPosition3, 1.0);
	vec4 p4 = WorldViewProjectionMatrix * vec4(VertexPosition4, 1.0);
	vec4 Position = p1;

	p1 /= p1.w;
	p2 /= p2.w;
	p3 /= p3.w;
	p4 /= p4.w;

	vec2 Line = p2.xy - p1.xy;
	vec2 Cond1 = p3.xy - p1.xy;
	vec2 Cond2 = p4.xy - p1.xy;

	float Cross1 = Line.x * Cond1.y - Line.y * Cond1.x;
	float Cross2 = Line.x * Cond2.y - Line.y * Cond2.x;

	if (Cross1 * Cross2 >= 0.0)
		gl_Position = Position;
	else
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
}
