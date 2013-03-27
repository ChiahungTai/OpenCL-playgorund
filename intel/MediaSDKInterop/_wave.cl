#define DIS 0.00005f
#define SP 0.7f
#define SP2 (SP*SP)
const sampler_t smp = CLK_FILTER_NEAREST; 
__kernel void ProcessUV(
                        __read_only image2d_t srcY, 
                        __read_only image2d_t srcUV, 
                        int srcPitch,
                        __write_only image2d_t dstY,
                        __write_only image2d_t dstUV,
                        int dstPitch,
                        int p1, 
                        int p2, 
                        __global int* pData)
{
    int x2 = get_global_id(0);
    int y2 = get_global_id(1);
    int w2 = get_global_size(0);
    int h2 = get_global_size(1);
    int	w4 = w2>>1;
    int	h4 = h2>>1;
    int	x4 = x2>>1;
    int	y4 = y2>>1;
    int2	pd = (int2)(x2,y2);
    if(x4>0 & x4<(w4-1) & y4>0 & y4<(h4-1))
    {
        int	f = pData[0]&1;
        __global float* pD = (__global float*)(pData+32);
        __global float*	pC = pD + (w4*h4)*f + x4+y4*w4;
        __global float*	pP = pD + (w4*h4)*(1-f) + x4+y4*w4;
        if((x2&1)&(y2&1))
        {// simulate waves
            float   L = 4*pC[0]-(pC[-1]+pC[+1]+pC[w4]+pC[-w4]);
            pP[0] = 2*pC[0]-pP[0]-SP2*L-DIS*p1*(pC[0]-pP[0]);
        }
        // calc optical distortion
        float x2r=0.5f*(x2&1);
        float y2r=0.5f*(y2&1);
        float dx2 = 0.25f*((pC[0]-pC[-1])*(1-x2r)*(1-y2r)+(pC[1]-pC[0])*(x2r)*(1-y2r)+
                           (pC[w4+0]-pC[w4-1])*(1-x2r)*(y2r)+(pC[w4+1]-pC[w4+0])*(x2r)*(y2r));
        float dy2 = 0.25f*((pC[0]-pC[-w4])*(1-x2r)*(1-y2r)+(pC[1]-pC[1-w4])*(x2r)*(1-y2r)+
                           (pC[w4]-pC[0])*(1-x2r)*(y2r)+(pC[w4+1]-pC[1])*(x2r)*(y2r));
        int2 ps = (int2)(clamp(x2+(int)dx2,0,w2-2), clamp(y2+(int)dy2,0,h2-2));
    
        // copy image data using optical distortion vector
		write_imagef(dstUV, pd, read_imagef(srcUV, smp, ps)); 
		pd *= 2;
		ps *= 2;
		int2	xy;
		xy = (int2)(0,0); write_imagef(dstY, pd+xy, read_imagef(srcY, smp, ps+xy)); 
		xy = (int2)(0,1); write_imagef(dstY, pd+xy, read_imagef(srcY, smp, ps+xy)); 
		xy = (int2)(1,0); write_imagef(dstY, pd+xy, read_imagef(srcY, smp, ps+xy)); 
		xy = (int2)(1,1); write_imagef(dstY, pd+xy, read_imagef(srcY, smp, ps+xy)); 
    }
}



__kernel void Mouse(int x, int y,int W,int H,int flag,int p1,int p2,__global int* pData)
{
    int	w4 = W>>2;
    int	h4 = H>>2;
    int	x4 = x>>2;
    int	y4 = y>>2;
    __global float*	pOut = (__global float*)(pData+32) + (w4*h4)*(pData[0]&1);
    pData[0]++;
    if(x<0 || y<0 || x>=W || y>=H) return;
    if(flag && pData[1] != x4 && pData[2] != y4)
    {// touch water
        int i,j;
        for(i=-50;i<=50;++i)for(j=-50;j<=50;++j)
        {
            float   r = (p2+1.0f)/30.0f;
            float   W = native_exp(-0.5f*(i*i+j*j)/(r*r));
            int     xx = x4+i;
            int     yy = y4+j;
            if(xx>=0 & xx<w4 & yy>=0 & yy<h4) pOut[xx+yy*w4] -= W*150; // start wave
        }
        pData[1] = x4; //save last position
        pData[2] = y4;
    }
}
