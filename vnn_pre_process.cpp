/****************************************************************************
*   Generated by ACUITY 5.11.0
*   Match ovxlib 1.1.21
*
*   Neural Network appliction pre-process source file
****************************************************************************/
/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jpeglib.h"
#include "vsi_nn_pub.h"
#include "vnn_global.h"
#include "vnn_pre_process.h"

#define _BASETSD_H

/*-------------------------------------------
                  Variable definitions
-------------------------------------------*/

/*{graph_input_idx, preprocess}*/
const static vsi_nn_preprocess_map_element_t* preprocess_map = NULL;

/*-------------------------------------------
                  Functions
-------------------------------------------*/
#define INPUT_META_NUM 1
static vnn_input_meta_t input_meta_tab[INPUT_META_NUM];
static void _load_input_meta()
{
    uint32_t i;
    for (i = 0; i < INPUT_META_NUM; i++)
    {
        memset(&input_meta_tab[i].image.preprocess,
            VNN_PREPRO_NONE, sizeof(int32_t) * VNN_PREPRO_NUM);
    }
    /* lid: input_27 */
    input_meta_tab[0].image.preprocess[0] = VNN_PREPRO_REORDER;
    input_meta_tab[0].image.preprocess[1] = VNN_PREPRO_MEAN;
    input_meta_tab[0].image.preprocess[2] = VNN_PREPRO_SCALE;
    input_meta_tab[0].image.reorder[0] = 0;
    input_meta_tab[0].image.reorder[1] = 1;
    input_meta_tab[0].image.reorder[2] = 2;
    input_meta_tab[0].image.mean[0] = 128.0;
    input_meta_tab[0].image.mean[1] = 128.0;
    input_meta_tab[0].image.mean[2] = 128.0;
    input_meta_tab[0].image.scale = 0.0078125;


}

static uint8_t *_float32_to_dtype
    (
    float *fdata,
    vsi_nn_tensor_t *tensor
    )
{
    vsi_status status;
    uint8_t *data;
    uint32_t sz,i,stride;

    sz = vsi_nn_GetElementNum(tensor);
    stride = vsi_nn_TypeGetBytes(tensor->attr.dtype.vx_type);
    data = (uint8_t *)malloc(stride * sz * sizeof(uint8_t));
    TEST_CHECK_PTR(data, final);
    memset(data, 0, stride * sz * sizeof(uint8_t));

    for(i = 0; i < sz; i++)
    {
        status = vsi_nn_Float32ToDtype(fdata[i], &data[stride * i], &tensor->attr.dtype);
        if(status != VSI_SUCCESS)
        {
            if(data)free(data);
            return NULL;
        }
    }

final:
    return data;
}

static float *_imageData_to_float32
    (
    uint8_t *bmpData,
    vsi_nn_tensor_t *tensor
    )
{
    float *fdata;
    uint32_t sz,i;

    fdata = NULL;
    sz = vsi_nn_GetElementNum(tensor);
    fdata = (float *)malloc(sz * sizeof(float));
    TEST_CHECK_PTR(fdata, final);

    for(i = 0; i < sz; i++)
    {
        fdata[i] = (float)bmpData[i];
    }

final:
    return fdata;
}


static void _data_scale
    (
    float *fdata,
    vnn_input_meta_t *meta,
    vsi_nn_tensor_t *tensor
    )
{
    uint32_t i,sz;
    float val,scale;

    sz = vsi_nn_GetElementNum(tensor);
    scale = meta->image.scale;
    if(0 != scale)
    {
        for(i = 0; i < sz; i++)
        {
            val = fdata[i] * scale;
            fdata[i] = val;
        }
    }
}

static void _data_mean
    (
    float *fdata,
    vnn_input_meta_t *meta,
    vsi_nn_tensor_t *tensor
    )
{
    uint32_t s0,s1,s2;
    uint32_t i,j,offset;
    float val,mean;

    s0 = tensor->attr.size[0];
    s1 = tensor->attr.size[1];
    s2 = tensor->attr.size[2];

    for(i = 0; i < s2; i++)
    {
        offset = s0 * s1 * i;
        mean = meta->image.mean[i];
        for(j = 0; j < s0 * s1; j++)
        {
            val = fdata[offset + j] - mean;
            fdata[offset + j ] = val;
        }
    }

}

/*
    caffe: transpose + reorder
    tf: reorder
*/
static void _data_transform
    (
    float *fdata,
    vnn_input_meta_t *meta,
    vsi_nn_tensor_t *tensor
    )
{
    uint32_t s0,s1,s2;
    uint32_t i,j,offset,sz,order;
    float *data;
    uint32_t *reorder;

    data = NULL;
    reorder = meta->image.reorder;
    s0 = tensor->attr.size[0];
    s1 = tensor->attr.size[1];
    s2 = tensor->attr.size[2];
    sz = vsi_nn_GetElementNum(tensor);
    data = (float *)malloc(sz * sizeof(float));
    TEST_CHECK_PTR(data, final);
    memset(data, 0, sizeof(float) * sz);

    for(i = 0; i < s2; i++)
    {
        if(s2 > 1 && reorder[i] <= s2)
        {
            order = reorder[i];
        }
        else
        {
            order = i;
        }

        offset = s0 * s1 * i;
        for(j = 0; j < s0 * s1; j++)
        {
            data[j + offset] = fdata[j * s2 + order];
        }
    }


    memcpy(fdata, data, sz * sizeof(float));
final:
    if(data)free(data);
}

static uint8_t *buffer_img = NULL;

void vnn_ReleaseBufferImage()
{
    if (buffer_img) free(buffer_img);
    buffer_img = NULL;
}

vsi_bool vnn_UseImagePreprocessNode()
{
    int32_t use_img_process;
    char *use_img_process_s;
    use_img_process = 0; /* default is 0 */
    use_img_process_s = getenv("VSI_USE_IMAGE_PROCESS");
    if(use_img_process_s)
    {
        use_img_process = atoi(use_img_process_s);
    }
    if (use_img_process)
    {
        return TRUE;
    }
    return FALSE;
}

const vsi_nn_preprocess_map_element_t * vnn_GetPrePorcessMap()
{
    return preprocess_map;
}

uint32_t vnn_GetPrePorcessMapCount()
{
    if (preprocess_map == NULL)
        return 0;
    else
        return sizeof(preprocess_map) / sizeof(vsi_nn_preprocess_map_element_t);
}