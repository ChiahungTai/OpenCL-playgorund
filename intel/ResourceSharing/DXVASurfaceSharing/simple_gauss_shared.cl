
// Copyright (c) 2009-2011 Intel Corporation
// All rights reserved.
// 
// WARRANTY DISCLAIMER
// 
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly


__kernel void simple_gauss_shared(__write_only image2d_t YOut, __read_only image2d_t YIn, __write_only image2d_t UVOut, __read_only image2d_t UVIn)
{
	
	const int x = get_global_id(0);		
	const int y = get_global_id(1);		

	float4 pixel = (float4)0;
	float4 color;

	int2 coord = (int2)(x, y); 
        const sampler_t smp = CLK_FILTER_NEAREST; 
	
	float guassianKernal[3][3] ={ 
			{0.09880393805197481, 0.11672306104983232, 0.09880393805197481}, 
			{0.11672306104983232, 0.13789200359277154, 0.11672306104983232}, 
			{0.09880393805197481, 0.11672306104983232, 0.09880393805197481} 
			};
	
	int kernalHeight=3;
	int kernalWidth=3;
	
	int kernalOffsetX=kernalWidth/2;
	int kernalOffsetY=kernalHeight/2;
		
	float R,G,B;

	float totalR,totalG,totalB;
	totalR=0;
	totalG=0;
	totalB=0;

	
	if(x == 0 || x == 1 || x == 639 || x == 640 || y ==0 || y ==1 || y==479 || y==480)
	{
			coord.x = x;
			coord.y = y;
			pixel = read_imagef(YIn, smp, coord); 
			
			totalR = totalR + pixel.x;
			totalG = totalG + pixel.y;
			totalB = totalB + pixel.z;
	}
	else
	{
		for(int i = 0;i<kernalHeight;i++)
		{
			for(int j=0;j<kernalWidth;j++)
			{
				coord.x = x -kernalOffsetX + j;
				coord.y = y -kernalOffsetY + i;
				pixel = read_imagef(YIn, smp, coord); 
				
				totalR = totalR + pixel.x*guassianKernal[j][i];
				totalG = totalG + pixel.y*guassianKernal[j][i];
				totalB = totalB + pixel.z*guassianKernal[j][i];
			}
		}
	}
	
	float4 finalColor;
	
	finalColor.x = totalR;
	finalColor.y = totalG;
	finalColor.z = totalB;
	finalColor.w = 0;
	coord.x=x;
	coord.y=y;
	write_imagef(YOut, coord, finalColor);

	totalR=0;
	totalG=0;
	totalB=0;
	
	int uvX = x/2;
	int uvY = y/2;
	
	if (x%2 == 0 && y%2 == 0)
	{
		if(x == 0 || x == 1 || x == 639 || x == 640 || y ==0 || y ==1 || y==479 || y==480)
		{
					coord.x = uvX;
					coord.y = uvY;
					pixel = read_imagef(UVIn, smp, coord); 
					
					totalR = totalR + pixel.x;
					totalG = totalG + pixel.y;
					totalB = totalB + pixel.z;
		
		}
		else
		{
			for(int i = 0;i<kernalHeight;i++)
			{
				for(int j=0;j<kernalWidth;j++)
				{
					coord.x = uvX -kernalOffsetX + j;
					coord.y = uvY -kernalOffsetY + i;
					pixel = read_imagef(UVIn, smp, coord);
		
					
					totalR = totalR + pixel.x*guassianKernal[j][i];
					totalG = totalG + pixel.y*guassianKernal[j][i];
					totalB = totalB + pixel.z*guassianKernal[j][i];
				}
			}
		}
		
		coord.x=uvX;
		coord.y=uvY;
		finalColor.x = totalR;
		finalColor.y = totalG;
		finalColor.z = totalB;
		write_imagef(UVOut, coord, finalColor);	
	}
}