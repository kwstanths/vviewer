#version 450

layout (location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputColor;
layout(set = 0, binding = 1) uniform sampler2D inputHighlight;

void main()
{
	outColor = texture(inputColor, inUV).rgba;
	vec4 highlightInfo = texture(inputHighlight, inUV).rgba;
	
	if (highlightInfo.r == 1.0f){
		vec2 size = 1.0f / textureSize(inputHighlight, 0);

        for (int i = -2; i <= +2; i++)
        {
            for (int j = -2; j <= +2; j++)
            {
                if (i == 0 && j == 0)
                {
                    continue;
                }

                vec2 offset = vec2(i, j) * size;

                if (texture(inputHighlight, inUV + offset).r == 0.0f)
                {
                    outColor = vec4(vec3(1.0f), 1.0f);
                    return;
                }
            }
        }
	}
}