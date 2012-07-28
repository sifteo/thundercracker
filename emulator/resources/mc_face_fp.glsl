varying vec2 texCoord;

uniform sampler2D normalmap;
uniform sampler2D lightmap;
uniform vec3 led;

const vec3 lightVec = vec3(-1.0, 2.0, 2.0);

const float specular = 0.002;
const float hardness = 8.0;
const float ambient = 0.16;

void main() {
    vec4 light = texture2D(lightmap, texCoord);
    vec3 normal = normalize((texture2D(normalmap, texCoord).rgb - vec3(0.5)) * vec3(2.0, 2.0, -2.0));

    float lambert = max(0.0, dot(normalize(lightVec), gl_NormalMatrix * normal));
    float luma = pow(lambert, hardness) * specular + light.b * ambient;

    vec3 color = vec3(luma) + light.g * led;

    gl_FragColor = vec4(color, 1.0);
}

