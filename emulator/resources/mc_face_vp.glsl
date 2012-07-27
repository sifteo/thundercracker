varying vec3 normal;
varying float depth;

const float HEIGHT = 0.9;

void main() {
     normal = gl_NormalMatrix * gl_Normal;
     depth = HEIGHT - gl_Vertex.z;

     gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
