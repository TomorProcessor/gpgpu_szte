
// TODO: Add OpenCL kernel code here.

__kernel void hello_kernel(__global const float *a,__global const float *b,__global float *result)
{
    int gid = get_global_id(0);

    result[gid] = a[gid] + b[gid];
}

__kernel void gaussian_filter(__read_only image2d_t srcImg,
                              __write_only image2d_t dstImg,
                              sampler_t sampler,
                              const int width, const int height, 
							  __constant float *const kernelWeights, 
							  const int kernelWidth)
{
    // Gaussian Kernel is:
    // 1  2  1
    // 2  4  2

//    float kernelWeights[9] = { 1.0f, 2.0f, 1.0f,
//                               2.0f, 4.0f, 2.0f,
//                               1.0f, 2.0f, 1.0f };

	int2 outImageCoord = (int2) (get_global_id(0), get_global_id(1));
    int2 startImageCoord = (int2) (outImageCoord.x - (kernelWidth-1)/2, outImageCoord.y - (kernelWidth-1)/2);
    int2 endImageCoord   = (int2) (outImageCoord.x + (kernelWidth-1)/2, outImageCoord.y + (kernelWidth+1)/2);
    

    if (outImageCoord.x < width && outImageCoord.y < height)
    {
        int weight = 0;
        float4 outColor = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
        for( int y = startImageCoord.y; y <= endImageCoord.y; y++)
        {
            for( int x = startImageCoord.x; x <= endImageCoord.x; x++)
            {
                outColor += (read_imagef(srcImg, sampler, (int2)(x, y)) * (kernelWeights[weight]));
                weight += 1;
            }
        }

        // Write the output value to image
        write_imagef(dstImg, outImageCoord, outColor);
    }
}

__kernel void rotation(__read_only image2d_t srcImg,__write_only image2d_t dstImg, sampler_t sampler) { 
		int2 outImageCoord = (int2) (get_global_id(0), get_global_id(1));
		int2 originOutImageCoord = (int2) (get_global_id(0), get_global_id(1));
		float angle = (3.14 / 180) * 30;
		int x = (outImageCoord.x-get_image_width(srcImg)/2) * cos(angle) - (outImageCoord.y-get_image_height(srcImg)/2) * sin(angle) +get_image_width(srcImg)/2;
		int y = (outImageCoord.x-get_image_width(srcImg)/2) * sin(angle) + (outImageCoord.y-get_image_height(srcImg)/2) * cos(angle) +get_image_height(srcImg)/2;
		//if (x > 0 && x < get_image_width(srcImg) && y > 0 && y < get_image_height(srcImg)) {
			outImageCoord.x = x;
			outImageCoord.y = y;
		//}
		
		float4 outColor = (float4)read_imagef(srcImg, sampler, (int2)originOutImageCoord);
		write_imagef(dstImg, outImageCoord, outColor);
}

__kernel void rotation_reverse(__read_only image2d_t srcImg,__write_only image2d_t dstImg, sampler_t sampler) { 
	int2 outImageCoord = (int2) (get_global_id(0), get_global_id(1));
		int2 originOutImageCoord = (int2) (get_global_id(0), get_global_id(1));
		float angle = (3.14 / 180) * 30;
		int x = (outImageCoord.x-get_image_width(srcImg)/2) * cos(angle) - (outImageCoord.y-get_image_height(srcImg)/2) * sin(angle) +get_image_width(srcImg)/2;
		int y = (outImageCoord.x-get_image_width(srcImg)/2) * sin(angle) + (outImageCoord.y-get_image_height(srcImg)/2) * cos(angle) +get_image_height(srcImg)/2;
		int out;
		if (x > 0 && x < get_image_width(srcImg) && y > 0 && y < get_image_height(srcImg)) {
			outImageCoord.x = x;
			outImageCoord.y = y;
			out = 0;
		} else {
			out = 1;
		}
		
		float4 outColor;
		if (!out) { 
			outColor = (float4)read_imagef(srcImg, sampler, (int2)outImageCoord);
		} else { 
			outColor = (float4)(1.0,0,0,0);
		}
		write_imagef(dstImg, originOutImageCoord, outColor);
}