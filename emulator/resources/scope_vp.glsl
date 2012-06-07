uniform float alphaAttr;

varying vec2 texCoord;
varying float alpha;

void main() {
    alpha = alphaAttr;
    texCoord = gl_MultiTexCoord0.xy;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
