#version 460 core

layout(location=0) in vec2 a_Position;

void main (void) {
    //  Turning the Right-angled triangle into a square quad

    //  1. Double it in size
    vec2 nm_pos = a_Position * 2;

    //  2. move the square region to enter the 0->1 for x and y -axes screen space render view
    nm_pos.y -= 1.;
    nm_pos.x += 1.;
    gl_Position = vec4(nm_pos, 0.0, 1.0);
}
