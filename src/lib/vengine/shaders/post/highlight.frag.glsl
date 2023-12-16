#version 450

layout (location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputColor;
layout(set = 0, binding = 1) uniform sampler2D inputHighlight;

void main()
{
    outColor = texture(inputColor, inUV).rgba;
    vec4 highlightInfo = texture(inputHighlight, inUV).rgba;
    
    if (highlightInfo.a > 0.5f){
        vec2 size = 1.0f / textureSize(inputHighlight, 0);

        for (int i = -3; i <= +3; i++)
        {
            for (int j = -3; j <= +3; j++)
            {
                if (i == 0 && j == 0)
                {
                    continue;
                }

                vec2 offset = vec2(i, j) * size;

                if (texture(inputHighlight, inUV + offset).a < 0.5f)
                {
                    outColor = vec4(vec3(highlightInfo.a), 1.0f);
                }
            }
        }
    }
}
