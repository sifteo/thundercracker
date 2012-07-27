varying vec3 normal;

// Light direction (somewhat consistent with background lightmap)
const vec3 lightVec = vec3(-1.0, 2.0, 2.0);

// Light strength/color
const vec4 diffuse = vec4(0.8, 0.8, 0.8, 1.0);
const vec4 ambient = vec4(0.2, 0.2, 0.2, 1.0);

void main() {
     float lambert = max(0.0, dot(normalize(lightVec), normalize(normal)));
     gl_FragColor = lambert * diffuse + ambient;
}

