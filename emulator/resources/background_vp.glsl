varying vec2 texCoord;
varying vec2 eyeCoord;

void main() {
     texCoord = gl_MultiTexCoord0.xy;

     vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
     gl_Position = pos;

     eyeCoord = pos.xy / pos.w * vec2(0.5, -0.5) + 0.5;
}
