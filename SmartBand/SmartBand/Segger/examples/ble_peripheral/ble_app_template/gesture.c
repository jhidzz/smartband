
#include <stdio.h>
#include "gesture.h"

const q7_t conv1_wt[CONV1_OUT_CH*CONV1_KX*CONV1_KY]=DS_CNN_conv_1_weights_0;
const q7_t conv1_bias[CONV1_OUT_CH]=DS_CNN_conv_1_biases_0;
const q7_t conv2_ds_wt[CONV1_OUT_CH*CONV2_DS_KX*CONV2_DS_KY]=DS_CNN_conv_ds_1_dw_conv_depthwise_weights_0;
const q7_t conv2_ds_bias[CONV1_OUT_CH]=DS_CNN_conv_ds_1_dw_conv_biases_0;
const q7_t conv2_pw_wt[CONV2_OUT_CH*CONV1_OUT_CH]=DS_CNN_conv_ds_1_pw_conv_weights_0;
const q7_t conv2_pw_bias[CONV2_OUT_CH]=DS_CNN_conv_ds_1_pw_conv_biases_0;
const q7_t conv3_ds_wt[CONV2_OUT_CH*CONV3_DS_KX*CONV3_DS_KY]=DS_CNN_conv_ds_2_dw_conv_depthwise_weights_0;
const q7_t conv3_ds_bias[CONV2_OUT_CH]=DS_CNN_conv_ds_2_dw_conv_biases_0;
const q7_t conv3_pw_wt[CONV3_OUT_CH*CONV2_OUT_CH]=DS_CNN_conv_ds_2_pw_conv_weights_0;
const q7_t conv3_pw_bias[CONV3_OUT_CH]=DS_CNN_conv_ds_2_pw_conv_biases_0;
const q7_t conv4_ds_wt[CONV3_OUT_CH*CONV4_DS_KX*CONV4_DS_KY]=DS_CNN_conv_ds_3_dw_conv_depthwise_weights_0;
const q7_t conv4_ds_bias[CONV3_OUT_CH]=DS_CNN_conv_ds_3_dw_conv_biases_0;
const q7_t conv4_pw_wt[CONV4_OUT_CH*CONV3_OUT_CH]=DS_CNN_conv_ds_3_pw_conv_weights_0;
const q7_t conv4_pw_bias[CONV4_OUT_CH]=DS_CNN_conv_ds_3_pw_conv_biases_0;
const q7_t conv5_ds_wt[CONV4_OUT_CH*CONV5_DS_KX*CONV5_DS_KY]=DS_CNN_conv_ds_4_dw_conv_depthwise_weights_0;
const q7_t conv5_ds_bias[CONV4_OUT_CH]=DS_CNN_conv_ds_4_dw_conv_biases_0;
const q7_t conv5_pw_wt[CONV5_OUT_CH*CONV4_OUT_CH]=DS_CNN_conv_ds_4_pw_conv_weights_0;
const q7_t conv5_pw_bias[CONV5_OUT_CH]=DS_CNN_conv_ds_4_pw_conv_biases_0;
const q7_t final_fc_wt[CONV5_OUT_CH*OUT_DIM]=DS_CNN_fc1_weights_0;
const q7_t final_fc_bias[OUT_DIM]=DS_CNN_fc1_biases_0;

q7_t scratch_pad[SCRATCH_BUFFER_SIZE];

q7_t* buffer1;
q7_t* buffer2;
q7_t* col_buffer;


void arm_avepool_q7_HWC_nonsquare (
        const q7_t * Im_in,           // input image
        const uint16_t dim_im_in_x,   // input image dimension
        const uint16_t dim_im_in_y,   // input image dimension
        const uint16_t ch_im_in,      // number of input image channels
        const uint16_t dim_kernel_x,  // window kernel size
        const uint16_t dim_kernel_y,  // window kernel size
        const uint16_t padding_x,     // padding sizes
        const uint16_t padding_y,     // padding sizes
        const uint16_t stride_x,      // stride
        const uint16_t stride_y,      // stride
        const uint16_t dim_im_out_x,  // output image dimension
        const uint16_t dim_im_out_y,  // output image dimension
        q7_t * bufferA,               // a buffer for local storage
        q7_t * Im_out,                // output feature
        const uint16_t out_lshift)    // output left shift (scaling)
{

  int16_t i_ch_in, i_x, i_y;
  int16_t k_x, k_y;
  
  for(i_ch_in=0;i_ch_in<ch_im_in;i_ch_in++) {
    for(i_y=0;i_y<dim_im_out_y;i_y++) {
      for(i_x=0;i_x<dim_im_out_x;i_x++) {
        int sum = 0;
        int count = 0;
        for (k_y = i_y*stride_y-padding_y; k_y < i_y*stride_y-padding_y+dim_kernel_y; k_y++) {
          for (k_x = i_x*stride_x-padding_x;k_x < i_x*stride_x-padding_x+dim_kernel_x; k_x++) {
            if (k_y >= 0 && k_x >= 0 && k_y<dim_im_in_y && k_x<dim_im_in_x) {
              sum += Im_in[i_ch_in + ch_im_in*(k_x+k_y*dim_im_in_x)];
              count++;
            }
          }
        }
        Im_out[i_ch_in+ch_im_in*(i_x+i_y*dim_im_out_x)] = sum*(0x1<<out_lshift)/count;
      }
    }
  }
}


void gesture_init()
{
    buffer1 = scratch_pad;
    buffer2 = buffer1 + (CONV1_OUT_CH*CONV1_OUT_X*CONV1_OUT_Y);
    col_buffer = buffer2 + (CONV2_OUT_CH*CONV2_OUT_X*CONV2_OUT_Y);
}


void gesture_recognition(q7_t* out_data, q7_t* in_data)
{
    //CONV1 : regular convolution
    arm_convolve_HWC_q7_basic_nonsquare(in_data, CONV1_IN_X, CONV1_IN_Y, 1, conv1_wt, CONV1_OUT_CH, CONV1_KX, CONV1_KY, CONV1_PX, CONV1_PY, CONV1_SX, CONV1_SY, conv1_bias, CONV1_BIAS_LSHIFT, CONV1_OUT_RSHIFT, buffer1, CONV1_OUT_X, CONV1_OUT_Y, (q15_t*)col_buffer, NULL);
    arm_relu_q7(buffer1,CONV1_OUT_X*CONV1_OUT_Y*CONV1_OUT_CH);

    //CONV2 : DS + PW conv
    //Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,CONV2_IN_X,CONV2_IN_Y,CONV1_OUT_CH,conv2_ds_wt,CONV1_OUT_CH,CONV2_DS_KX,CONV2_DS_KY,CONV2_DS_PX,CONV2_DS_PY,CONV2_DS_SX,CONV2_DS_SY,conv2_ds_bias,CONV2_DS_BIAS_LSHIFT,CONV2_DS_OUT_RSHIFT,buffer2,CONV2_OUT_X,CONV2_OUT_Y,(q15_t*)col_buffer, NULL);
    arm_relu_q7(buffer2,CONV2_OUT_X*CONV2_OUT_Y*CONV2_OUT_CH);
    //Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2, CONV2_OUT_X, CONV2_OUT_Y, CONV1_OUT_CH, conv2_pw_wt, CONV2_OUT_CH, 1, 1, 0, 0, 1, 1, conv2_pw_bias, CONV2_PW_BIAS_LSHIFT, CONV2_PW_OUT_RSHIFT, buffer1, CONV2_OUT_X, CONV2_OUT_Y, (q15_t*)col_buffer, NULL);
    arm_relu_q7(buffer1,CONV2_OUT_X*CONV2_OUT_Y*CONV2_OUT_CH);

    //CONV3 : DS + PW conv
    //Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,CONV3_IN_X,CONV3_IN_Y,CONV2_OUT_CH,conv3_ds_wt,CONV2_OUT_CH,CONV3_DS_KX,CONV3_DS_KY,CONV3_DS_PX,CONV3_DS_PY,CONV3_DS_SX,CONV3_DS_SY,conv3_ds_bias,CONV3_DS_BIAS_LSHIFT,CONV3_DS_OUT_RSHIFT,buffer2,CONV3_OUT_X,CONV3_OUT_Y,(q15_t*)col_buffer, NULL);
    arm_relu_q7(buffer2,CONV3_OUT_X*CONV3_OUT_Y*CONV3_OUT_CH);
    //Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2, CONV3_OUT_X, CONV3_OUT_Y, CONV2_OUT_CH, conv3_pw_wt, CONV3_OUT_CH, 1, 1, 0, 0, 1, 1, conv3_pw_bias, CONV3_PW_BIAS_LSHIFT, CONV3_PW_OUT_RSHIFT, buffer1, CONV3_OUT_X, CONV3_OUT_Y, (q15_t*)col_buffer, NULL);
    arm_relu_q7(buffer1,CONV3_OUT_X*CONV3_OUT_Y*CONV3_OUT_CH);

    //CONV4 : DS + PW conv
    //Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,CONV4_IN_X,CONV4_IN_Y,CONV3_OUT_CH,conv4_ds_wt,CONV3_OUT_CH,CONV4_DS_KX,CONV4_DS_KY,CONV4_DS_PX,CONV4_DS_PY,CONV4_DS_SX,CONV4_DS_SY,conv4_ds_bias,CONV4_DS_BIAS_LSHIFT,CONV4_DS_OUT_RSHIFT,buffer2,CONV4_OUT_X,CONV4_OUT_Y,(q15_t*)col_buffer, NULL);
    arm_relu_q7(buffer2,CONV4_OUT_X*CONV4_OUT_Y*CONV4_OUT_CH);
    //Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2, CONV4_OUT_X, CONV4_OUT_Y, CONV3_OUT_CH, conv4_pw_wt, CONV4_OUT_CH, 1, 1, 0, 0, 1, 1, conv4_pw_bias, CONV4_PW_BIAS_LSHIFT, CONV4_PW_OUT_RSHIFT, buffer1, CONV4_OUT_X, CONV4_OUT_Y, (q15_t*)col_buffer, NULL);
    arm_relu_q7(buffer1,CONV4_OUT_X*CONV4_OUT_Y*CONV4_OUT_CH);


    //CONV5 : DS + PW conv
    //Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,CONV5_IN_X,CONV5_IN_Y,CONV4_OUT_CH,conv5_ds_wt,CONV4_OUT_CH,CONV5_DS_KX,CONV5_DS_KY,CONV5_DS_PX,CONV5_DS_PY,CONV5_DS_SX,CONV5_DS_SY,conv5_ds_bias,CONV5_DS_BIAS_LSHIFT,CONV5_DS_OUT_RSHIFT,buffer2,CONV5_OUT_X,CONV5_OUT_Y,(q15_t*)col_buffer, NULL);
    arm_relu_q7(buffer2,CONV5_OUT_X*CONV5_OUT_Y*CONV5_OUT_CH);
    //Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2, CONV5_OUT_X, CONV5_OUT_Y, CONV4_OUT_CH, conv5_pw_wt, CONV5_OUT_CH, 1, 1, 0, 0, 1, 1, conv5_pw_bias, CONV5_PW_BIAS_LSHIFT, CONV5_PW_OUT_RSHIFT, buffer1, CONV5_OUT_X, CONV5_OUT_Y, (q15_t*)col_buffer, NULL);
    arm_relu_q7(buffer1,CONV5_OUT_X*CONV5_OUT_Y*CONV5_OUT_CH);

    //Average pool
    arm_avepool_q7_HWC_nonsquare(buffer1,CONV5_OUT_X,CONV5_OUT_Y,CONV5_OUT_CH,CONV5_OUT_X,CONV5_OUT_Y,0,0,1,1,1,1,NULL,buffer2, 2);

    arm_fully_connected_q7(buffer2, final_fc_wt, CONV5_OUT_CH, OUT_DIM, FINAL_FC_BIAS_LSHIFT, FINAL_FC_OUT_RSHIFT, final_fc_bias, out_data, (q15_t*)col_buffer);
    
    arm_softmax_q7(out_data, OUT_DIM, out_data);
}
