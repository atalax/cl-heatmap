

float2 tile_to_cartesian(float2 pt, float4 trx, float4 try)
{
	float4 at = (float4) (
		pt.x,
		pt.y,
		1.0,
		0.0
	);
	float2 ret = (float2) (
		dot(at, trx),
		dot(at, try)
	);
	return ret;
}


bool in_bounding_box(float2 in, float4 box)
{
	return in.x >= box.x && in.x <= box.z && in.y <= box.y && in.y >= box.w;
}
