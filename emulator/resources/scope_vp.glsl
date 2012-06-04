attribute vec4 gl_Color;

varying vec2 texCoord;
varying float alpha;

void main() {
    alpha = gl_Color.a;
    texCoord = gl_MultiTexCoord0.xy;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
