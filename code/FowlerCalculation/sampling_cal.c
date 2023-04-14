#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

#define FRAME_X 2048   
#define FRAME_Y 2048

#define CDS_MODE    1
#define CDSNOISE_MODE   2
#define FOWLER_MODE 3

//rm -f sampling_cal.o sampling_cal.so
//g++ -fPIC -c sampling_cal.c
//g++ -shared -o libsampling_cal.so sampling_cal.o

extern "C"
{
    float* fowler_calculation(int mode, int f_num, int count, unsigned short *indata)
    {
        //printf("%d %d %d\r\n", mode, f_num, count);
    
        //float* findata = (float*)malloc(FRAME_X*FRAME_Y*sizeof(float)*count);
        //memset(findata, 0, FRAME_X*FRAME_Y*sizeof(float)*count);
        float* findata = new float[FRAME_X*FRAME_Y*count];
        
        for(int i = 0; i<FRAME_X*FRAME_Y*count; i++){
            //printf("[%d] %d >>> ", i, indata[i]);
            findata[i] = (float)indata[i];
            //printf("%f\r\n", findata[i]);
        }

        int ramps = 1, groups = 1, reads = f_num, channel = 32;

        if(mode == CDS_MODE){
            ramps = 1;  groups = 1; reads *= 2;
        }
        else if(mode == CDSNOISE_MODE){
            ramps = 2; groups = 1;  reads *= 2;
        }
        else if(mode == FOWLER_MODE){
            ramps = 1; groups = 2;  
        }

        //float** img = (float **)malloc(count*sizeof(float));
        //for(int idx = 0 ; idx < count ; idx++)
        //    img[idx] = (float*)malloc(FRAME_X*FRAME_Y*sizeof(float));
        float** img = new float*[count];
        for(int idx = 0 ; idx < count ; idx++)
            img[idx] = new float[FRAME_X*FRAME_Y];


        for(int idx = 0 ; idx < count ; idx++)
            memcpy(img[idx], &findata[FRAME_X*FRAME_Y*idx], FRAME_X*FRAME_Y*sizeof(float));

        //for(int i = 0 ; i < FRAME_X*FRAME_Y ; i++)
        //    printf("[%d] %f-%f, %f-%f\r\n", i, findata[i], img[0][i], findata[FRAME_X*FRAME_Y + i], img[1][i]);

        //free(findata);
        delete findata;

        //for 64pix    
        int ch_pix_cnt = (int)(FRAME_X / channel);

        //printf("1. referece pixel subtraction !!!\r\n");
        //---------------------------------------------------------------
        //1. refer pixel subtraction
        int frame = 0;
        for (int r = 0; r < ramps; r++){
            for (int g = 0; g < groups; g++) {
                for (int n = 0; n < reads; n++) {

                    //1.1. top, bottom average
                    for (int ch = 0; ch < channel; ch++)
                    {
                        long long TopBottomSum = 0;
                        float aver = 0;

                        for (int row = 0; row < 4; row++)
                        {
                            for (int col = 0; col < ch_pix_cnt; col++)
                                TopBottomSum += (int)img[frame][(FRAME_X * row) + (ch * ch_pix_cnt + col)];
                        }

                        for (int row = FRAME_Y - 4; row < FRAME_Y; row++)
                        {
                            for (int col = 0; col < ch_pix_cnt; col++) 
                                TopBottomSum += (int)img[frame][(FRAME_X * row) + (ch * ch_pix_cnt + col)];
                        }

                        aver = (float)TopBottomSum / (ch_pix_cnt * 8);

                        for (int row = 0; row < FRAME_Y; row++)
                        {
                            for (int col = 0; col < ch_pix_cnt; col++)
                                img[frame][(FRAME_X * row) + (ch * ch_pix_cnt + col)] -= (float)aver;
                        }
                    }

                    //1.2. row average
                    int ROW_MULTI = 4;  //9line

                    //float *img_tmp = (float*)malloc(FRAME_X*FRAME_Y*sizeof(float));
                    float *img_tmp = new float[FRAME_X*FRAME_Y];
                    for (int row = 0; row < FRAME_Y; row++)
                        for (int col = 0; col < FRAME_X; col++)
                            img_tmp[FRAME_X * row + col] = img[frame][FRAME_X * row + col];

                    for (int row = 0; row < FRAME_Y; row++)
                    {
                        long double RowSum = 0;
                        float aver = 0;

                        int startRow = 0;
                        int endRow = 0;
                        if (row - ROW_MULTI < 0)
                            startRow = 0;
                        else
                            startRow = row - ROW_MULTI;

                        if (row + ROW_MULTI > FRAME_Y - 1)
                            endRow = FRAME_Y - 1;
                        else
                            endRow = row + ROW_MULTI;

                        for (int row2 = startRow; row2 <= endRow; row2++) {
                            for (int col = 0; col < 4; col++)
                                RowSum += img[frame][(FRAME_X * row2) + col];
                            for (int col = FRAME_X - 4; col < FRAME_X; col++)
                                RowSum += img[frame][(FRAME_X * row2) + col];
                        }

                        aver = (float)RowSum / (8 * (endRow - startRow + 1));

                        for (int col = 0; col < FRAME_X; col++)
                            img_tmp[(FRAME_X * row) + col] -= (float)aver;
                    }
                    for (int row = 0; row < FRAME_Y; row++)
                        for (int col = 0; col < FRAME_X; col++)
                            img[frame][FRAME_X * row + col] = img_tmp[FRAME_X * row + col];

                    frame++; 

                    //free(img_tmp);
                    delete img_tmp;
                }
            }
        } 
        
        int row=2;
        int col=2;
        //("%d, %d: img0-%f, img1-%f, img2-%f, img3-%f\r\n", row, col, img[0][FRAME_X * row + col], img[1][FRAME_X * row + col], img[2][FRAME_X * row + col], img[3][FRAME_X * row + col]);

        //printf("2. sampling calculation !!!\r\n");

        //---------------------------------------------------------------
        //2. fowler calculationsqrt
        //result
        if(mode == CDS_MODE){

            //float* res = (float*)malloc(FRAME_X * FRAME_Y*sizeof(float));
            float* res = new float[FRAME_X*FRAME_Y];
            for (int row = 0; row < FRAME_Y; row++)
                for (int col = 0; col < FRAME_X; col++){
                    res[(FRAME_X * row) + col] = (float)(img[1][(FRAME_X * row) + col] - img[0][(FRAME_X * row) + col]);
                    //printf("%f - %f = %f\r\n", img[1][(FRAME_X * row) + col], img[0][(FRAME_X * row) + col], res[(FRAME_X * row) + col]);
                    //if(FRAME_X * row + col <= 10 || FRAME_X * row + col >= FRAME_X*FRAME_Y-10)
                    //    printf("%d: %f\r\n", FRAME_X * row + col, res[(FRAME_X * row) + col]);
                }
            
            //free(img[0]);
            //free(img[1]);
            //free(img); 
            delete img[0];
            delete img[1];
            delete img;
            //printf("size: %d\r\n", FRAME_X * FRAME_Y*sizeof(float));

            //printf("Finished !!!\r\n");

            return res;
        }
        else if (mode == CDSNOISE_MODE){

            //float* res = (float*)malloc(FRAME_X*FRAME_Y*sizeof(float)*3);
            float* res = new float[FRAME_X*FRAME_Y*3];

            //S_n - P_n
            for (int r = 0; r < ramps; r++) {
                for (int row = 0; row < FRAME_Y; row++) {
                    for (int col = 0; col < FRAME_X; col++){
                        res[(FRAME_X*FRAME_Y*r) + (FRAME_X * row) + col] = img[ramps*r + 1][(FRAME_X * row) + col] - img[ramps*r][(FRAME_X * row) + col];
                        //if(row==2 && col==2)
                        //    printf("%d, %d %d: %f = %f - %f\r\n", r, row, col, res[(FRAME_X*FRAME_Y*r) + (FRAME_X * row) + col], img[ramps*r + 1][(FRAME_X * row) + col], img[ramps*r][(FRAME_X * row) + col]);
                    }
                }
            }

            //printf("1\r\n");

            for (int i = 0; i < 4; i++)
                //free(img[i]);
                delete img[i];
            //free(img); 
            delete img;

            //printf("2\r\n");

            //for 1 result
            for (int row = 0; row < FRAME_Y; row++) {
                for (int col = 0; col < FRAME_X; col++){
                    res[(FRAME_X*FRAME_Y*2) + (FRAME_X * row) + col] = (res[(FRAME_X*FRAME_Y) + (FRAME_X * row) + col] - res[(FRAME_X * row) + col]) / sqrt(2);
                    //if(row==2 && col==2)
                    //    printf("%d, %d: %f = %f - %f\r\n", row, col, res[(FRAME_X*FRAME_Y*2) + (FRAME_X * row) + col], res[(FRAME_X*FRAME_Y) + (FRAME_X * row) + col], res[(FRAME_X * row) + col]);
                }
            }

            //printf("Finished !!!\r\n");

            return res;
        }
        else if(mode == FOWLER_MODE){

            //S_n - P_n
            //float* res = (float*)malloc(FRAME_X * FRAME_Y * sizeof(float));
            //memset(res, 0, sizeof(float)* (FRAME_X* FRAME_Y));
            float* res = new float[FRAME_X*FRAME_Y];
            for (int n = 0; n < reads ; n++) {
                for (int row = 0; row < FRAME_Y; row++) {
                    for (int col = 0; col < FRAME_X; col++)
                    {
                        if(n == 0)  res[(FRAME_X * row) + col] = 0;
                        res[(FRAME_X * row) + col] += img[reads + n][(FRAME_X * row) + col] - img[n][(FRAME_X * row) + col];
                    }
                }
            }

            for (int i = 0; i < reads * 2; i++){
                //free(img[i]); 
                delete img[i];
                //printf("Memory released!");
            }

            //free(img); 
            delete img;

            //printf("Main Memory released!");

            for (int row = 0; row < FRAME_Y; row++) {
                for (int col = 0; col < FRAME_X; col++)
                    res[(FRAME_X * row) + col] /= reads;
            }

            //printf("Finished !!!\r\n");

            return res;
        }
        
        return 0;
    }
}

