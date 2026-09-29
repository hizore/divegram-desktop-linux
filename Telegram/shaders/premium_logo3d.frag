#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec2 vUV;
layout(location = 2) in vec3 vObjPos;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D u_Texture;
layout(binding = 2) uniform sampler2D u_NormalMap;

layout(std140, binding = 0) uniform Params {
	mat4 mvp;
	mat4 world;
	vec4 grad1;
	vec4 grad2;
	vec4 params;
	vec4 extra;
};

float goldenSpec(vec3 lightPos, vec3 pos, vec3 norm, vec3 eyeDir, float k) {
	vec3 refl = reflect(-normalize(lightPos - pos), norm);
	return clamp(k * pow(max(dot(eyeDir, refl), 0.0), 2.0), 0.0, 1.0);
}

void main() {
	float alpha = extra.y;
	vec3 gradientColor1 = grad1.rgb;
	vec3 gradientColor2 = grad2.rgb;

	vec3 pos = vObjPos / 100.0 + 0.5;
	float gradientMix = clamp(distance(pos.xy, vec2(1.0, 1.0)), 0.0, 1.0);
	vec3 color = mix(gradientColor1, gradientColor2, gradientMix);

	vec3 norm = normalize(vec3(world * vec4(vNormal, 0.0)));
	vec3 eyeDir = vec3(0.0, 0.0, 1.0);

	float diffuse = max(dot(norm, normalize(vec3(-3.0, -3.0, 20.0) - vObjPos)), 0.0);
	color = mix(vec3(0.0), color, 0.75 + 0.35 * diffuse);

	float spec = 0.0;
	spec += goldenSpec(vec3(-1.0, 0.7, 0.2), vObjPos, norm, eyeDir, 2.0) / 5.0;
	spec += goldenSpec(vec3(8.0, 0.7, 0.5), vObjPos, norm, eyeDir, 2.0) / 5.0;
	spec += goldenSpec(vec3(-3.0, -3.0, 0.5), vObjPos, norm, eyeDir, 2.0) / 4.0;
	spec += goldenSpec(vec3(4.0, 3.0, 2.5), vObjPos, norm, eyeDir, 1.5) / 6.0;
	spec = clamp(spec, 0.0, 0.8);
	vec3 specColor = gradientColor1 * 1.8;
	color = mix(color, specColor, spec);

	vec3 flecksNormal = normalize(1.0 - texture(
		u_NormalMap,
		(vUV + vec2(-params.x, params.x)) * 2.0).xyz);
	float flecksSpec = goldenSpec(vec3(1.2, -0.2, 0.5), vObjPos, norm, eyeDir, 2.0);
	color += clamp(flecksSpec * abs(vNormal.z) * flecksNormal.z, 0.0, 0.3)
		* specColor * 0.5;

	float rim = pow(1.0 - max(dot(norm, eyeDir), 0.0), 2.0);
	color += gradientColor2 * rim * 0.25;

	fragColor = vec4(color * alpha, alpha);
}