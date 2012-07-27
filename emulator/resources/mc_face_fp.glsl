varying vec3 normal;
varying float depth;

const vec3 lightVec = vec3(-1.0, 2.0, 2.0);

const float specular = 0.6;
const float ambient = 0.15;
const float occlusion = 1.0;
const float gamma = 2.2;

void main() {
     float lambert = clamp(dot(normalize(lightVec), normalize(normal)), 0.0, 1.0);
     float luma = (lambert * lambert) * specular + ambient - depth * occlusion;
     float linear = pow(luma, gamma);
     gl_FragColor = vec4(linear, linear, linear, 1.0);
}

