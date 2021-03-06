//------------------------------------------------------------------------------
//  simple.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

mat4 ShapeModel;
//vec4 MatDiffuse;
//vec4 MatAnotherColor;
float HeightMultiplier;
sampler2D HeightMap;
sampler2D TextureMask_1;
sampler2D Texture_1;
sampler2D Texture_2;
sampler2D Texture_3;
sampler2D Texture_4;
float Tile_1_Scale;
float Tile_2_Scale;
float Tile_3_Scale;
float Tile_4_Scale;
float TerrainSize;

samplerstate TileSampler
{
	Samplers = { Texture_1, Texture_2, Texture_3, Texture_4, HeightMap};
	//Filter = Point;
	//Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
};

samplerstate HeightSampler
{
	Samplers = {HeightMap};
	//Filter = Point;
	//Filter = MinMagMipLinear;
	AddressU = Mirror;
	AddressV = Mirror;
};

state WireframeState
{
	CullMode = None;	
	BlendEnabled[0] = true;	
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	FillMode = Line;
	//MultisampleEnabled = true;
};

state DepthEnabledState
{
	CullMode = None;	
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;	
	DstBlend[0] = OneMinusSrcAlpha;
	DepthEnabled = true;
	DepthWrite = true;
	MultisampleEnabled = true;
};

state DepthDisabledState
{
	CullMode = None;	
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	DepthEnabled = false;
	DepthWrite = false;
	//MultisampleEnabled = true;
};


//------------------------------------------------------------------------------
/**
*/
shader
void
vsMainShape(in vec2 position, out vec2 UV, out vec4 normals, out float Height, out vec3 ViewSpacePos)  
{
	UV = position / TerrainSize;
	float height = texture(HeightMap, UV).r;
	vec4 offset = vec4(position.x, height * HeightMultiplier, position.y, 1.0f);
	gl_Position = ViewProjection * Model * offset;
	
	Height = height;
	
	mat4 modelView = View * Model;
	ViewSpacePos = (modelView * offset).xyz;
	// # P.xy store the position for which we want to calculate the normals
	// # height() here is a function that return the height at a point in the terrain

	// read neightbor heights using an arbitrary small offset
	vec3 off = vec3(1.0, 1.0, 0.0);
	float hL = texture(HeightMap, (offset.xz - off.xz)/TerrainSize).r;
	float hR = texture(HeightMap, (offset.xz + off.xz)/TerrainSize).r;
	float hD = texture(HeightMap, (offset.xz - off.zy)/TerrainSize).r;
	float hU = texture(HeightMap, (offset.xz + off.zy)/TerrainSize).r;

	// deduce terrain normal
	vec3 Normal;
	Normal.x = hL - hR;
	Normal.y = hD - hU;
	Normal.z = 2.0;
	Normal = normalize(Normal);
	
	normals = PackViewSpaceNormal(Normal);
}
	
//------------------------------------------------------------------------------
/**
*/
shader
void
psMainShape(in vec2 UV, in vec4 normals, in float Height, in vec3 ViewSpacePos, [color0] out vec4 Color, [color1] out vec4 Normals, [color2] out float Depth) 
{	
	float4 alphas = texture(TextureMask_1, UV).rgba;
	
	vec3 color1 = texture(Texture_1, UV*Tile_1_Scale).rgb;
	vec3 color2 = texture(Texture_2, UV*Tile_2_Scale).rgb;
	vec3 color3 = texture(Texture_3, UV*Tile_3_Scale).rgb;
	vec3 color4 = texture(Texture_4, UV*Tile_4_Scale).rgb;
	color1 = color1*alphas.r;
	color2 = color2*alphas.g;
	color3 = color3*alphas.b;
	color4 = color4*alphas.a;
	
	Color = float4(color1+color2+color3+color4,1);
	//Color = vec4(UV*alphas,0,1);
	Normals = normals;
	Depth = length(ViewSpacePos);
}

SimpleTechnique(DepthShape, "MyStatic", vsMainShape(), psMainShape(), DepthEnabledState);
SimpleTechnique(WireframeShape, "MyStatic|Alt0", vsMainShape(), psMainShape(), WireframeState);