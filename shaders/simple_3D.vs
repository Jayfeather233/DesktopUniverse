#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 ourColor;

uniform dmat4 model;
uniform dmat4 view3D;
uniform dmat4 projection;

void main() {
  dvec4 pr = projection * view3D * model * dvec4(aPos, 1.0);
  // dvec4 pr2 = pr / pr.w;
  gl_Position = vec4(pr.xy/pr.w, log(float(pr.z))/100 ,1.0);
  ourColor = aColor;
}
