#version 430 core

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

uniform sampler2D u_texture;
uniform float u_time;

// Basic random function
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// Create a single moving droplet with trail
vec4 movingDroplet(vec2 uv, float time) {
    vec2 grid = vec2(6.0, 3.0) * 3.0;
    vec2 id = floor(uv * grid);

    // Random movement parameters
    float speed = 0.5 + rand(id) * 0.5;
    float xOffset = (rand(id * 1.37) - 0.5) * 0.3;
    float startTime = rand(id * 2.45) * 10.0;

    // Vertical movement with time
    vec2 st = fract(uv * grid) - vec2(0.5, 0.0);
    st.y += mod(time * speed + startTime, 2.0) - 1.0;

    // Droplet center position with slight wiggle
    float wiggle = sin(time * 2.0 + id.x * 10.0) * 0.05;
    vec2 dropPos = vec2(xOffset + wiggle, 0.5);

    // Distance to droplet center
    float dropDist = length(st - dropPos);

    // Main droplet sharper
    float drop = smoothstep(0.1, 0.0, dropDist);

    // Trail (narrower and shorter)
    float trail = smoothstep(0.15, 0.0, abs(st.x - dropPos.x)) *
        smoothstep(0.0, 0.3, st.y - dropPos.y);

    return vec4(drop, trail, 0.0, 0.0);
}

void main() {
    vec2 uv = fragCoord;

    // Create moving droplets
    vec4 droplets = vec4(0.0);
    for (int i = 0; i < 2; i++) {
        float scale = 1.0 + float(i) * 0.15;
        droplets += movingDroplet(uv * scale, u_time * (1.0 + float(i) * 0.2));
    }

    // Combine drop and trail
    float dropletEffect = min(1.0, droplets.x * 0.8 + droplets.y * 0.5);

    // Compute offset for distortion based on droplet center
    vec2 offset = vec2(0.0);
    if (dropletEffect > 0.0) {
        vec2 center = vec2(0.5);
        vec2 direction = normalize(uv - center);
        offset = direction * dropletEffect * 0.01;
    }

    // Apply UV distortion
    vec2 distortedUV = uv + offset;

    vec3 blurredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 sampleUV = distortedUV + vec2(x, y) * 0.002; // Blur strength
            float weight = (x == 0 && y == 0) ? 4.0 : 1.0;
            blurredColor += texture(u_texture, sampleUV).rgb * weight;
            totalWeight += weight;
        }
    }
    blurredColor /= totalWeight;

    // Fetch the sharp original texture
    vec3 sharpColor = texture(u_texture, distortedUV).rgb;

    // Blend based on droplet strength
    vec3 color = mix(sharpColor, blurredColor, dropletEffect);

    // Darken overall scene slightly
    color *= 0.65;

    // tint it slightly blue for rainy vibe
    color = mix(color, vec3(0.6, 0.7, 0.8), 0.1); //

    // Add inner highlight (fake specular light in droplet)
    float highlight = smoothstep(0.08, 0.0, length(uv - vec2(0.5))) * dropletEffect * 0.8;
    color += highlight;

    fragColor = vec4(color, 1.0);
}
