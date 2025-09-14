
struct VS_INPUT
{
    float3 Position : POSITION;
};

struct VS_OUTPUT
{
    float3 Position : SV_POSITION;
};

float4 main( float4 pos : POSITION ) : SV_POSITION
{
	return pos;
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    output.Position = input.Position;

    return output;
}