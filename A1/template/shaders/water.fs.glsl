#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float dead_timer;

in vec2 uv;

layout(location = 0) out vec4 color;

vec2 distort(vec2 uv) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// HANDLE THE WATER WAVE DISTORTION HERE (you may want to try sin/cos)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    float a = 0.1;
    float b = 0.03;
    float t = 4.0;
    float s = 4.0;
    
    float x = uv.x;    
    float y = uv.y;
    
    x += sin(t*y + s*x + cos(a * time)) * 0.01;
    y += sin(t*y + s*x + cos(a * time)) * 0.01;

    return vec2(x, y);
}

vec4 color_shift(vec4 in_color) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// HANDLE THE COLOR SHIFTING HERE (you may want to make it blue-ish)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    vec4 color = in_color + 
        mix(vec4(0.1, 0.1, 0.3, 0.1), vec4(0.15, 0.15, 0.35, 0.2), (sin(time / 10.0) / 2.0) + 1.0);
	return color;
}

vec4 fade_color(vec4 in_color) 
{
	vec4 color = in_color;
	if (dead_timer > 0)
		color -= 0.1 * dead_timer * vec4(0.1, 0.1, 0.1, 0);

	return color;
}

void main()
{
	vec2 coord = distort(uv);

    vec4 in_color = texture(screen_texture, coord);
    
    color = color_shift(in_color);
    color = fade_color(color);
}
