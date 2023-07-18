

#include "image_processing.h"

#include "dis_camera.h"
#include "message.h"
#include "HF_Double_DC_Motor.h"
#include <stdint.h>


uint8_t flag_show_status = 0;
#define  _avg_fre 4
static uint8_t _threshold_avg[_avg_fre] = {0};
static uint8_t _pos = 0;
uint8_t threshold_avg(uint8_t threshold){
    _threshold_avg[_pos] = threshold;
    _pos += 1;
    if (_pos >= _avg_fre)_pos = 0;
    uint8_t pos = _pos + _avg_fre;
    uint16_t ans = 0;
    for (uint8_t i = 1; i <= _avg_fre; ++i) {
        ans += _threshold_avg[(pos-i)%_avg_fre];
    }
    return ans/_avg_fre;
}

/**
*args:
*
*histogram：长度为256的整型数组，表示图像中0~255出现的像素值的个数。
*
*pixel_total：整型变量，表示图像中像素的总数。
*
*https://www.ipol.im/pub/art/2016/158/article_lr.pdf
*
*/
uint8_t otsu_threshold( uint8_t* histogram, int pixel_total ){
    //用于计算均值
    unsigned int sumB = 0;
    unsigned int sum1 = 0;
    //用于计算类间方差
    float wB = 0.0f;
    float wF = 0.0f;
    float mF = 0.0f;
    //用于记录最大的类间方差
    float max_var = 0.0f;
    //用于计算类间方差
    float inter_var = 0.0f;
    //返回值：表示计算得到的阈值
    uint8_t threshold = 0;
    //索引buf
    int index_histo = 0;

    for ( index_histo = 0; index_histo < 256; ++index_histo ){
        sum1 += index_histo * histogram[ index_histo ];
    }

    for (index_histo = 0; index_histo < 256; ++index_histo){
        wB += histogram[ index_histo ];
        if ( wB == 0 ){
            continue;
        }
        wF = pixel_total - wB;
        sumB += index_histo * histogram[ index_histo ];
        mF = ( sum1 - sumB ) / wF;
        inter_var = wB * wF * ( ( sumB / wB ) - mF ) * ( ( sumB / wB ) - mF );
        if ( inter_var >= max_var ){
            threshold = index_histo;
            max_var = inter_var;
        }
    }

    //threshold = threshold_avg(threshold);

    if(flag_show_status){
        sprintf(buf, " %d ", threshold);
        show_right_bottom_message(buf);
    }
    
    return threshold;
}

uint8_t otsu( uint8_t* histogram, int pixel_total ){
    uint8_t threshold;
    //用于计算均值
    uint32_t sum = 0;
    //索引buf
    int index_histo = 0;

    uint32_t max, min;
    uint32_t pixel_back = 0;
    uint32_t pixel_fore = 0;
    uint32_t pixel_integral_back = 0;
    uint32_t pixel_integral_fore = 0;
    double OmegaBack, OmegaFore, MicroBack, MicroFore, SigmaB = -1.0, Sigma; // 类间方差; 

    for ( index_histo = 0; index_histo < 256; ++index_histo ){
        sum += index_histo * histogram[ index_histo ];
    }

    for(min = 0; min < 256 && histogram[min] == 0; ++min);
    for(max = 255; max > 0 && histogram[max] == 0; --max);

    for ( index_histo = min; index_histo < max; ++index_histo ) {

        pixel_back = pixel_back + histogram[ index_histo ];
        pixel_fore = pixel_total - pixel_back;

        OmegaBack = pixel_back*1.0 / pixel_total;
        OmegaFore = pixel_fore*1.0 / pixel_total;

        pixel_integral_back += histogram[ index_histo ] * index_histo;
        pixel_integral_fore = sum - pixel_integral_back;

        MicroBack = pixel_integral_back*1.0 / pixel_back;
        MicroFore = pixel_integral_fore*1.0 / pixel_fore;

        Sigma = OmegaBack * OmegaFore * (MicroBack - MicroFore) * (MicroBack - MicroFore);

        if (Sigma > SigmaB) {
            SigmaB = Sigma;
            threshold = index_histo;
        }
    }
    
    return threshold;
}

uint8_t get_threshold() {
    uint8_t histogram[256] = {0};

    for (int i = 0; i < c_h; ++i) {
        uint8_t *one_h = mt9v03x_image_dvp[i];
        for (int j = 0; j < c_w; ++j) {
            histogram[one_h[j]]++;
        }
    }

    //return otsu_threshold(histogram, c_h * c_w);
    return otsu(histogram, c_h * c_w);
}