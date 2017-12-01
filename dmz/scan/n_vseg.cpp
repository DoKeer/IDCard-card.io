//
//  n_vseg.cpp
//  See the file "LICENSE.md" for the full license governing this code.
//

#include "compile.h"
#if COMPILE_DMZ

#include "n_vseg.h"
#include "eigen.h"
#include "dmz.h"

#include "cv/morph.h"
#include "cv/convert.h"

#include "models/generated/modelm_befe75da.hpp"
// TODO: gpu for matrix mult?

enum {
  NumberPatternUnknown = 0,
  NumberPatternVisalike = 1,
  NumberPatternAmexlike = 2,
  //lgq 
  NumberPatternThreeSix = 3,
  NumberPatternSixThirteen = 4,
  NumberPatternNineTeen = 5,

};

static uint8_t const NumberLengthForNumberPatternType[3] = {0, 16, 15};
static uint8_t const NumberPatternLengthForPatternType[3] = {0, 19, 17};
//lgq 尝试增加储蓄卡支持
//static uint8_t const NumberLengthForNumberPatternType[5] = {0, 16, 15, 18, 19};
//static uint8_t const NumberPatternLengthForPatternType[4] = {0, 19, 17, 20};

static uint8_t const NumberPatternUnknownPattern[19]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
////lgq
//static uint8_t const NumberPatternThreeSixPattern[20] = {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1};
////lgq
//static uint8_t const NumberPatternSixThirteenPattern[20] = {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
////lgq
//static uint8_t const NumberPatternNineTeenPattern[19] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

static uint8_t const NumberPatternVisalikePattern[19] = {1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1};
static uint8_t const NumberPatternAmexlikePattern[19] = {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0};
//lgq 3->6
//static uint8_t const * NumberPatternForPatternType[6] = {NumberPatternUnknownPattern, NumberPatternVisalikePattern, NumberPatternAmexlikePattern ,NumberPatternThreeSixPattern, NumberPatternSixThirteenPattern, NumberPatternNineTeenPattern};

static uint8_t const * NumberPatternForPatternType[6] = {NumberPatternUnknownPattern, NumberPatternVisalikePattern, NumberPatternAmexlikePattern };

typedef Eigen::Matrix<float, 1, 204, Eigen::RowMajor> VSegModelInput;
typedef Eigen::Matrix<float, 1, 3, Eigen::RowMajor> VSegProbabilities;

#define kVertSegSumWindowSize 27

DMZ_INTERNAL inline VSegProbabilities vseg_probabilities_for_hstrip(IplImage *y, IplImage *cropped_gradient, IplImage *downsampled_normed, IplImage *as_float) {
  
  //lgq 裁剪三次
  llcv_morph_grad3_1d_u8(y, cropped_gradient);
  llcv_lineardown2_1d_u8(cropped_gradient, downsampled_normed);
  llcv_norm_convert_1d_u8_to_f32(downsampled_normed, as_float);
  
  Eigen::Map<VSegModelInput> vseg_model_input((float *)as_float->imageData);
  
  VSegProbabilities probabilities = applym_befe75da(vseg_model_input);
//  std::cerr << ">>>>lgq probabilities ==  " <<probabilities << "\n";

  return probabilities;
}

DMZ_INTERNAL inline void best_segmentation_for_vseg_scores(float *visalike_scores, float *amexlike_scores, NVerticalSegmentation *best) {
  float visalike_sum = 0.0f;
  float amexlike_sum = 0.0f;
  // Why a ring buffer? As an efficient means of doing a box window convolution.
  // I tried re-summing the ring buffer each time instead of doing a running sum, to
  // see whether there were issues with accumulated errors. There were not, so I'm
  // leaving this bit of premature optimization in. :)
  float visalike_ring_buffer[kVertSegSumWindowSize];
  float amexlike_ring_buffer[kVertSegSumWindowSize];
  
  best->score = 0.0f;
  best->pattern_type = NumberPatternUnknown;
  best->y_offset = 0;
  
  for(uint16_t y_offset = 0; y_offset < 270; y_offset++) {
    float visalike_score = visalike_scores[y_offset];
    float amexlike_score = amexlike_scores[y_offset];

    visalike_sum += visalike_score;
    amexlike_sum += amexlike_score;
//    std::cerr << ">>>>lgq visalike_sum ==  " <<visalike_sum << "\n"<< "amexlike_sum == " << amexlike_sum << "\n";

    uint8_t buffer_index = y_offset % kVertSegSumWindowSize;//lgq y_offset %27 取整
    visalike_ring_buffer[buffer_index] = visalike_score;
    amexlike_ring_buffer[buffer_index] = amexlike_score;
    
    if(y_offset >= kVertSegSumWindowSize - 1) {// 比较更像哪一种银行卡
      // ring buffer is full, start using the scores
      if(visalike_sum > best->score) {
//        std::cerr << ">>>>lgq visalike_sum ==  " <<visalike_sum << "\n";

        best->score = visalike_sum;
        best->pattern_type = NumberPatternVisalike;
        best->y_offset = y_offset - kVertSegSumWindowSize + 1;
      }
      if(amexlike_sum > best->score) {
//        std::cerr << "lgq amexlike_sum == " << amexlike_sum << "\n";
        best->score = amexlike_sum;
        best->pattern_type = NumberPatternAmexlike;
        best->y_offset = y_offset - kVertSegSumWindowSize + 1;
      }
      
      uint8_t next_buffer_index = (y_offset + 1) % kVertSegSumWindowSize;
      visalike_sum -= visalike_ring_buffer[next_buffer_index];
      amexlike_sum -= amexlike_ring_buffer[next_buffer_index];
    }
  }
}

DMZ_INTERNAL NVerticalSegmentation best_n_vseg(IplImage *y) {
  assert(y->roi == NULL);
  CvSize y_size = cvGetSize(y);
#pragma unused(y_size) // work around broken compiler warnings
  assert(y_size.width == kCreditCardTargetWidth);
  assert(y_size.height == kCreditCardTargetHeight);
  assert(y->depth == IPL_DEPTH_8U);
  assert(y->nChannels == 1);

  // Set up reusable memory for image calculations lgq 设置图像计算的可重用内存
  IplImage *cropped_gradient = cvCreateImage(cvSize(408, 1), IPL_DEPTH_8U, 1);//裁剪梯度
  IplImage *downsampled_normed = cvCreateImage(cvSize(204, 1), IPL_DEPTH_8U, 1);//向下抽样规范
  IplImage *as_float = cvCreateImage(cvSize(204, 1), IPL_DEPTH_32F, 1);  //作为浮动

  // Score buffers, to be filled in as needed
  float visalike_scores[270];
  //lgq memset函数用来将指定 visa类型的卡片 内存的前size个字节设置为特定的值
  memset(visalike_scores, 0, sizeof(visalike_scores));
  float amexlike_scores[270];
  memset(amexlike_scores, 0, sizeof(amexlike_scores));
//lgq<
  float threeSix_scores[270];
  memset(threeSix_scores, 0, sizeof(threeSix_scores));
  
  float sixThirteen_scores[270];
  memset(sixThirteen_scores, 0, sizeof(sixThirteen_scores));
  
  float nineteen_scores[270];
  memset(nineteen_scores, 0, sizeof(nineteen_scores));
  //>
  
  
  
  uint16_t min_y_offset = 0;
  uint16_t max_y_offset = 270;
  uint8_t y_offset_step = 4;
  
  //lgq y方向遍历图片 最初计算每y_offset_step步长，缩小我们必须工作的地区 到卡号区域 对应型号的 scores 接近 1，需要添加其他卡支持。
  
  // Initially, calculate every fourth score, to narrow down the area in which we have to work
  for(uint16_t y_offset = min_y_offset; y_offset < max_y_offset; y_offset += y_offset_step) {
    cvSetImageROI(y, cvRect(10, y_offset, 408, 1));//lgq 为什么高度是1？期望区域
    //lgq
    VSegProbabilities probabilities = vseg_probabilities_for_hstrip(y, cropped_gradient, downsampled_normed, as_float);

    visalike_scores[y_offset] = probabilities(0, 1);
    amexlike_scores[y_offset] = probabilities(0, 2);
    //lgq <
    threeSix_scores[y_offset] = probabilities(0, 0);
    sixThirteen_scores[y_offset] = probabilities(0, 0);
    nineteen_scores[y_offset] = probabilities(0, 0);
    //>
//    std::cerr << ">>>>lgq y_offset ==  " <<y_offset << "\n";
//    
//    std::cerr << ">>>>lgq probabilities(0, 0) ==  " <<probabilities(0, 0) << "\n";
//
//    std::cerr << ">>>>lgq probabilities(0, 1) ==  " <<probabilities(0, 1) << "\n";
//
//    std::cerr << ">>>>lgq probabilities(0, 2) ==  " <<probabilities(0, 2) << "\n";

  }

  NVerticalSegmentation best;
  best_segmentation_for_vseg_scores(visalike_scores, amexlike_scores, &best);//lgq竖向图像分割 垂直最佳分割 分数

//  std::cerr << ">>>>lgq best.y_offset ==  " <<best.y_offset << "\n";

  // Now that we know roughly where we're interested in, fill in a few more scores
  // (the ones that could make a difference), and recalculate

#define kFineTuningBuffer 8
  // The scores that matter are (roughly) y_offset - kFineTuningBuffer : y_offset + kVertSegSumWindowSize + kFineTuningBuffer
  // In theory, due to windowed summing, the scores in the middle also don't matter (since they would count equally
  // for all possible y_offsets), but go ahead and calculate them anyway, since the provide useful signal about
  // whether it is actually a credit card present or not

  // All values must be bounds checked against 270 and (when needed) safely against 0 (using uints!)
  
  //lgq 算出卡号所在的y方向宽度  min_y_offset < width <max_y_offset
  min_y_offset = MIN(270, best.y_offset < kFineTuningBuffer ? 0 : best.y_offset - kFineTuningBuffer);
  max_y_offset = MIN(270, best.y_offset + kVertSegSumWindowSize + kFineTuningBuffer);
  y_offset_step = 1;

  for(uint16_t y_offset = min_y_offset; y_offset < max_y_offset; y_offset += y_offset_step) {
    // Don't recalculate anything -- we already calculated 1/4th of them!不要重新计算任何东西 - 我们已经计算了1/4的！
    if(visalike_scores[y_offset] == 0 && amexlike_scores[y_offset] == 0) {
      cvSetImageROI(y, cvRect(10, y_offset, 408, 1));
      VSegProbabilities probabilities = vseg_probabilities_for_hstrip(y, cropped_gradient, downsampled_normed, as_float);
      visalike_scores[y_offset] = probabilities(0, 1);
      amexlike_scores[y_offset] = probabilities(0, 2);
      
      
//      std::cerr << ">>>>lgq recalculate y_offset ==  " <<y_offset << "\n";
//      
//      std::cerr << ">>>>lgq recalculate probabilities(0, 1) ==  " <<probabilities(0, 1) << "\n";
//      
//      std::cerr << ">>>>lgq recalculate probabilities(0, 2) ==  " <<probabilities(0, 2) << "\n";
    }
  }

  // TODO: Hint that resumming across all the possible values isn't really necessary...
  best_segmentation_for_vseg_scores(visalike_scores, amexlike_scores, &best); //lgq 再比较一遍 best

  cvReleaseImage(&cropped_gradient);
  cvReleaseImage(&downsampled_normed);
  cvReleaseImage(&as_float);
  cvResetImageROI(y);

  best.number_pattern_length = NumberPatternLengthForPatternType[best.pattern_type]; //找到对应类型的卡号长度 lgq
  //lgq 分配对应类型的卡号长度的内存 number_pattern 初始化分配19个大小空间，应该改成20位
  memcpy(&best.number_pattern, NumberPatternForPatternType[best.pattern_type], sizeof(best.number_pattern));
  //
  best.number_length = NumberLengthForNumberPatternType[best.pattern_type];

  return best;
}


#endif // COMPILE_DMZ
