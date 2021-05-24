#version 330 core

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec3 u_ambient_light;
uniform mat4 u_shadow_viewproj;
uniform float u_shadow_bias;

uniform vec4 u_color;
uniform vec3 u_light_position;
uniform vec3 u_light_vector;
uniform vec3 u_light_color;

uniform int u_light_type;

uniform float u_light_cutoff;
uniform float u_light_maxdist;
uniform float u_light_intensity;

uniform sampler2D u_texture;
uniform sampler2D shadowmap;

uniform float u_time;
uniform float u_alpha_cutoff;

float shadow_fact(vec4 v_lightspace_position)
{
	//from homogeneus space to clip space
	vec2 shadow_uv = v_lightspace_position.xy / v_lightspace_position.w;

	//from clip space to uv space
	shadow_uv = shadow_uv * 0.5 + vec2(0.5);

	//get point depth [-1 .. +1] in non-linear space
	float real_depth = (v_lightspace_position.z - u_shadow_bias) / v_lightspace_position.w;
	real_depth = real_depth * 0.5 + 0.5;

	if( shadow_uv.x < 0.0 || shadow_uv.x > 1.0 || shadow_uv.y < 0.0 || shadow_uv.y > 1.0 ){
 		return 1.0;
	}
	
	bool u_pcf = true;
	float shadow_factor = 0.0;
	if (u_pcf)
	{
		vec2 texel_size = 1.0 / textureSize(shadowmap, 0);
		for(int x = -1; x <= 1; ++x)
		{
  			for(int y = -1; y <= 1; ++y)
  			{	
        			float shadow_depth = texture(shadowmap, shadow_uv.xy + vec2(x, y) * texel_size).x; 

				if( shadow_depth < real_depth ) { shadow_factor += 0.0; }
				else { shadow_factor += 1.0; }    
    			}    
		}
		shadow_factor /= 9.0;
	}
	else
	{
		float shadow_depth = texture(shadowmap, shadow_uv.xy).x; 
		//we can compare them, even if they are not linear
		if( shadow_depth < real_depth ) { shadow_factor += 0.0; }
		else { shadow_factor += 1.0; }  
	}
	return shadow_factor; 
}

void main()
{	
	vec3 light = vec3( 0.0 );
	vec2 uv = v_uv;
	vec4 color = u_color;
	color *= texture2D( u_texture, v_uv );

	if(color.a < u_alpha_cutoff)
		discard;	

	//very important to normalize as they come
	//interpolated so normalization is lost
	vec3 N = normalize( v_normal );

	//if the light is a directional light the light
	//vector is the same for all pixels
	//we assume the vector is normalized
	vec3 L;
	
	light += u_ambient_light;
	
	if (u_light_type == 2)
	{
		//depending on the light type...
		vec4 v_lightspace_position = u_shadow_viewproj * vec4(v_world_position, 1.0);			
		L = normalize(-u_light_vector);
		
		//compute how much is aligned
		float NdotL = dot(N,L);

		//light cannot be negative (but the dot product can)
		NdotL = clamp( NdotL, 0.0, 1.0 );

		float shadow_factor = shadow_fact(v_lightspace_position);

		//store the amount of diffuse light
		light += u_light_intensity * (NdotL * u_light_color) * shadow_factor;
	}
	else
	{	
		//Defining the light vector
		L = u_light_position - v_world_position;

		//Compute distance and define the attenuation factor
		float light_distance = length( L );
		float att_factor;

		//compute a linear attenuation factor
		att_factor = u_light_maxdist - light_distance;

		//normalize factor
		att_factor /= u_light_maxdist;

		//ignore negative values
		att_factor = max( att_factor, 0.0 );

		//Normalizing L for the point and spot light dot products
		L = normalize(L);

		//compute how much is aligned
		float NdotL = dot(N,L);

		//light cannot be negative (but the dot product can)
		NdotL = clamp( NdotL, 0.0, 1.0 );

		//store the amount of diffuse light
		light += u_light_intensity * (NdotL * u_light_color) * att_factor;
	}

	color.xyz *= light;

	gl_FragColor = color;
}
