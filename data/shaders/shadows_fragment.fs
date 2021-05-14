#version 330 core

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec3 u_ambient_light;
uniform mat4 u_shadow_viewproj;

uniform vec4 u_color;
uniform vec3 u_light_position;
uniform vec3 u_light_vector;
uniform vec3 u_light_color;
uniform vec3 u_spotDirection;

uniform float u_light_cutoff;
uniform int u_light_type;
uniform float u_light_maxdist;
uniform float u_light_intensity;
uniform float u_spotExponent;

uniform sampler2D u_texture;
uniform sampler2D shadowmap;

uniform float u_time;
uniform float u_alpha_cutoff;

float shadow_fact(vec4 v_lightspace_position)
{
	//from homogeneus space to clip space
	vec3 shadow_uv = v_lightspace_position.xyz / v_lightspace_position.w;

	//from clip space to uv space
	shadow_uv = shadow_uv * 0.5 + vec3(0.5);

	//get point depth [-1 .. +1] in non-linear space
	float u_shadow_bias = 0.001;
	
	float real_depth = shadow_uv.z;

	float shadow_factor = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowmap, 0);
	for(float x = -1.5; x <= 1.5; ++x)
	{
  		for(float y = -1.5; y <= 1.5; ++y)
  		{
        		float shadow_depth = texture(shadowmap, shadow_uv.xy + vec2(x, y) * texelSize).x; 

			//we can compare them, even if they are not linear
			if( shadow_depth < real_depth - u_shadow_bias ) { shadow_factor += 0.0; }
			else { shadow_factor += 1.0; }    
    		}    
	}
	shadow_factor /= 16.0;

	if( shadow_uv.x < 0.0 || shadow_uv.x > 1.0 || shadow_uv.y < 0.0 || shadow_uv.y > 1.0 ){
      		if (u_light_type == 2) {return 1.0;}
		else {return 0.0;}
	}

	//it is before near or behind far plane
	if(real_depth < 0.0 || real_depth > 1.0){
   		if (u_light_type == 2) {return 1.0;}
		else {return 0.0;}
	}

	return shadow_factor; 
}

void main()
{	
	vec3 light = vec3( 0.0 );
	vec2 uv = v_uv;
	vec4 color = u_color;
	color *= texture( u_texture, v_uv );

	if(color.a < u_alpha_cutoff)
		discard;	

	//very important to normalize as they come
	//interpolated so normalization is lost
	vec3 N = normalize( v_normal );

	//if the light is a directional light the light
	//vector is the same for all pixels
	//we assume the vector is normalized
	vec3 L;
	
	light += vec3(1,1,1);//u_ambient_light;

	//depending on the light type...
	vec4 v_lightspace_position = u_shadow_viewproj * vec4(v_world_position, 1.0);			
	L = normalize(-u_spotDirection);
		
	//compute how much is aligned
	float NdotL = dot(N,L);

	//light cannot be negative (but the dot product can)
	NdotL = clamp( NdotL, 0.0, 1.0 );

	float shadow_factor = shadow_fact(v_lightspace_position);

	//store the amount of diffuse light
	light += u_light_intensity * (NdotL * u_light_color) * shadow_factor;

	color.xyz *= light;

	gl_FragColor = color; //vec4(vec3(abs(N.x), abs(N.y), abs(N.z)), 1.0);
}
