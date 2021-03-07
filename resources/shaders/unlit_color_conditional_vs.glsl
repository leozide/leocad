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

	if (((p1.y - p2.y) * (p3.x - p1.x) + (p2.x - p1.x) * (p3.y - p1.y)) * ((p1.y - p2.y) * (p4.x - p1.x) + (p2.x - p1.x) * (p4.y - p1.y)) >= 0.0)
	{
		if (((p2.y - p1.y) * (p3.x - p2.x) + (p1.x - p2.x) * (p3.y - p2.y)) * ((p2.y - p1.y) * (p4.x - p2.x) + (p1.x - p2.x) * (p4.y - p2.y)) >= 0.0)
			gl_Position = Position;
		else
			gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
	}
	else
		gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
}
