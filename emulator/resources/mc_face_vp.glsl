varying vec2 texCoord;

void main() {
    texCoord = vec2(0.5, 0.5) - gl_Vertex.xy * vec2(0.25, 0.5);
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
