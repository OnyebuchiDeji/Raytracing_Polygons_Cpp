#version 460 core

// For version 460 core, gl_FragColor is removed.
// It onmly works on some NVIDIA drivers as a compatibility
// fallback but is not portable and will hard-fail on AMD/Intel or stricter contexts
layout(location=0) out vec4 FragColor;

void main()
{
	FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}