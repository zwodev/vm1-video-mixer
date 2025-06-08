#version 310 es
precision mediump float;

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D inputTexture;

/*
void main() {
    const float y_scale = 1080.0f / 1632.0f;
    const float y_offset = 1092.0f / 1632.0f;
    vec2 texCoordY = vec2(texCoord.x, texCoord.y);
    float y = texture(inputTexture, texCoordY).r;
    fragColor = vec4(y, 0.0f, 0.0f, 1.0f);
}
*/

void main() {
    
	//vec2 uv = vec2((xNew / IN_TEX_SIZE.x) + D.x, (yNew / IN_TEX_SIZE.y) + D.y);

    const float y_scale = 1080.0f / 1632.0f;
    const float y_offset = 1088.0f / 1632.0f;
    const float uv_step = 1.0f / 128.0f;

    float x = floor(floor(texCoord.x * 128.0f) / 2.0);
    vec2 texCoordY = vec2(texCoord.x, texCoord.y * y_scale);
    vec2 texCoordU = vec2(x * uv_step * 2.0f + 0.5 * uv_step, y_offset + texCoordY.y * 0.5f);
    vec2 texCoordV = vec2(texCoordU.x + uv_step, texCoordU.y);

    float y = texture(inputTexture, texCoordY).r;
    float u = texture(inputTexture, texCoordU).r - 0.5f;
    float v = texture(inputTexture, texCoordV).r - 0.5f;
	
	float r = y + (1.403f * v);
	float g = y - (0.344f * u) - (0.714f * v);
	float b = y + (1.770f * u);

    fragColor = vec4(r, g, b, 1.0f);
}