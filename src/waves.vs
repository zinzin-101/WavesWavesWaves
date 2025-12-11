#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 FragPos;
out vec3 Normal;

uniform vec3 camOffset;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define NUM_OF_SINE_WAVES 36

uniform float time;
uniform float amplitude[NUM_OF_SINE_WAVES];
uniform float wavelength[NUM_OF_SINE_WAVES];
uniform vec3 direction[NUM_OF_SINE_WAVES];
uniform float speed[NUM_OF_SINE_WAVES];

void main()
{
    vec3 pos = aPos;
    pos.x += camOffset.x;
    pos.z += camOffset.z;

    float height = 0.0;
    float dx = 0.0;
    float dz = 0.0;

    float b_a = 1.0;
    float b_f = 1.0;

    for (int i = 0 ; i < NUM_OF_SINE_WAVES; i++){
        vec3 dir = normalize(direction[i]);
        float frequency = 2.0 / wavelength[i];
        float phase = speed[i] * (2.0 / wavelength[i]);

        float a = b_a * amplitude[i];
        float f = b_f * frequency;

        //float a = amplitude[i];
        //float f = frequency;

        height += a * exp(sin(((dir.x * pos.x + dir.z * pos.z) + dx + dz) * f + time * phase) - 1.0);
        dx += f * dir.x * a * cos(((dir.x * pos.x + dir.z * pos.z) + dx + dz) * f + time * phase) * exp(sin(((dir.x * pos.x + dir.z * pos.z) + dx + dz) * f + time * phase) - 1.0);
        dz += f * dir.z * a * cos(((dir.x * pos.x + dir.z * pos.z) + dx + dz) * f + time * phase) * exp(sin(((dir.x * pos.x + dir.z * pos.z) + dx + dz) * f + time * phase) - 1.0);

        b_a *= 0.92;
        b_f *= 1.08;
    }

    vec3 normal = normalize(vec3(-dx, 1.0, -dz));
    pos.y = height;
    vec3 realPos = aPos;
    realPos.y = pos.y;
    FragPos = vec3(model * vec4(realPos, 1.0));
    //Normal = mat3(transpose(inverse(model))) * aNormal;
    Normal = mat3(transpose(inverse(model))) * normal;
    //Normal = mat3(transpose(inverse(model))) * vec3(0,1,0);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}