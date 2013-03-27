#define R 400.0f
#define R2 (R*R)
const sampler_t smp = CLK_FILTER_NEAREST; 
__kernel void ProcessUV(
                        __read_only image2d_t srcY, 
                        __read_only image2d_t srcUV, 
                        int srcPitch,
                        __write_only image2d_t dstY,
                        __write_only image2d_t dstUV,
                        int dstPitch,
                        int param1, 
                        int param2, 
                        __global float2* pD)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int w = get_global_size(0);
    int h = get_global_size(1);
    float2	p = (float2)(x,y);
    float2	d = (2.0f*p-pD[0]);
    float2  param = pD[1];
    int2	coordUV = (int2)(x,y);
    int2	coordY[4] = {(int2)(2*x,2*y),(int2)(2*x+1,2*y),(int2)(2*x,2*y+1),(int2)(2*x+1,2*y+1)};
    
    float4 UV,Y0,Y1,Y2,Y3;
    UV = read_imagef(srcUV, smp, coordUV); 
    Y0 = read_imagef(srcY, smp, coordY[0]); 
    Y1 = read_imagef(srcY, smp, coordY[1]); 
    Y2 = read_imagef(srcY, smp, coordY[2]); 
    Y3 = read_imagef(srcY, smp, coordY[3]); 
    
    if(dot(d,d)<R2)
    {
		UV.x = (UV.x-0.5f)*param.y+0.5f;
		UV.y = (UV.y-0.5f)*param.y+0.5f;
		Y0.x = (Y0.x-0.5f)*param.x+0.5f;
		Y1.x = (Y1.x-0.5f)*param.x+0.5f;
		Y2.x = (Y2.x-0.5f)*param.x+0.5f;
		Y3.x = (Y3.x-0.5f)*param.x+0.5f;
	}
	
    write_imagef(dstUV, coordUV, UV); 
    write_imagef(dstY, coordY[0],Y0); 
    write_imagef(dstY, coordY[1],Y1); 
    write_imagef(dstY, coordY[2],Y2); 
    write_imagef(dstY, coordY[3],Y3); 
}

__kernel void Mouse(int x, int y,int W,int H,int flag,int p1,int p2,__global float2* pD)
{
    if(flag) 
    {
        pD[0] = (float2)(x,y);
    }
    pD[1].x = (float)p1/(float)(1<<5);
    pD[1].y = (float)p2/(float)(1<<5);
}
