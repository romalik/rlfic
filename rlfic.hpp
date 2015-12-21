#ifndef RLFIC_HPP_
#define RLFIC_HPP_

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <deque>
#include <map>
#include <stdint.h>
#include <algorithm>
#include <limits.h>
#include <stdint.h>
#include <string.h>
class Mat {
public:
    std::vector<uint32_t> data;
    int channels;
    int rows;
    int cols;


    int cx;
    int cy;
    int attrs;

    static inline uint32_t getR(uint32_t val) {
        return (val & 0x00ff0000) >> 16;
    }
    static inline uint32_t getG(uint32_t val) {
        return (val & 0x0000ff00) >> 8;
    }
    static inline uint32_t getB(uint32_t val) {
        return (val & 0x000000ff);
    }


    Mat(int _rows, int _cols, int _channels) {
        rows = _rows;
        cols = _cols;
        channels = _channels;

        cx = 0;
        cy = 0;
        attrs = 0;

        data.resize(rows*cols, 0);
        data.reserve(rows*cols);

    }

    Mat() {
        channels = 0;
        rows = 0;
        cols = 0;
        attrs = 0;
        data.clear();
    }

    inline uint32_t& at(int row, int col) {
        //if(row >= 0 && row < rows && col >= 0 && col < cols) {
            return data[row*cols + col];
        //}
    }

    static Mat * imread(char* filename) //Achtung! BMP only!!
    {
        FILE* f = fopen(filename, "rb");
        unsigned char info[54];
        fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

        // extract image height and width from header
        int width = *(int*)&info[18];
        int height = *(int*)&info[22];

        int size = 3 * width * height;
        Mat * newMat = new Mat(height, width, 3);
        unsigned char* data = new unsigned char[size]; // allocate 3 bytes per pixel
        fread(data, sizeof(unsigned char), size, f); // read the rest of the data at once
        fclose(f);


        for(int i = 0; i < size; i += 3)
        {
            uint32_t px = 0;
            px = ((data[i+2] & 0xff) << 16) | ((data[i+1] & 0xff) << 8) | (data[i] & 0xff);
            newMat->data[i/3] = px;
        }

        return newMat;
    }


    void imwrite(char * path) { //Bmp only
        FILE *f;
        unsigned char *img = NULL;
        int filesize = 54 + 3*this->cols*this->rows;  //w is your image width, h is image height, both int

        img = (unsigned char *)malloc(3*this->cols*this->rows);
        memset(img,0,sizeof(img));

        for(int i=0; i<this->cols; i++)
        {
            for(int j=0; j<this->rows; j++)
            {
                int r = getR(this->at(j,i));
                int g = getG(this->at(j,i));
                int b = getB(this->at(j,i));

                img[(i+j*this->cols)*3+2] = (unsigned char)(r);
                img[(i+j*this->cols)*3+1] = (unsigned char)(g);
                img[(i+j*this->cols)*3+0] = (unsigned char)(b);
            }
        }
        unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
        unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
        unsigned char bmppad[3] = {0,0,0};

        bmpfileheader[ 2] = (unsigned char)(filesize    );
        bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
        bmpfileheader[ 4] = (unsigned char)(filesize>>16);
        bmpfileheader[ 5] = (unsigned char)(filesize>>24);

        bmpinfoheader[ 4] = (unsigned char)(       cols    );
        bmpinfoheader[ 5] = (unsigned char)(       cols>> 8);
        bmpinfoheader[ 6] = (unsigned char)(       cols>>16);
        bmpinfoheader[ 7] = (unsigned char)(       cols>>24);
        bmpinfoheader[ 8] = (unsigned char)(       rows    );
        bmpinfoheader[ 9] = (unsigned char)(       rows>> 8);
        bmpinfoheader[10] = (unsigned char)(       rows>>16);
        bmpinfoheader[11] = (unsigned char)(       rows>>24);

        f = fopen(path,"wb");
        fwrite(bmpfileheader,1,14,f);
        fwrite(bmpinfoheader,1,40,f);
        for(int  i=0; i<rows; i++)
        {
            fwrite(img+(cols*(i)*3),3,cols,f);
            fwrite(bmppad,1,(4-(cols*3)%4)%4,f);
        }
        fclose(f);

    }


    Mat * crop(int srcR, int srcC, int w, int h) {
        Mat * res = new Mat(h, w, channels);
        for(int r = 0; r < h; r++) {
            for(int c = 0; c < w; c++) {
                res->at(r,c) = this->at(srcR + r, srcC + c);
            }
        }
        return res;
    }

    enum rotate_type {
        MAT_ROTATE_0 = 0,
        MAT_ROTATE_90,
        MAT_ROTATE_180,
        MAT_ROTATE_270,
        MAT_ROTATE_CNT
    };

    void rotate(int rt) {
        Mat * newMat = new Mat(this->rows, this->cols, this->channels);
        if(rt == MAT_ROTATE_90) {
            for(int c = 0; c<this->cols; c++) {
                for(int r = 0; r<this->rows; r++) {
                    newMat->at(this->rows - c - 1, this->cols - r - 1) = this->at(r,c);
                }
            }
        } else if(rt == MAT_ROTATE_180) {
            for(int c = 0; c<this->cols; c++) {
                for(int r = 0; r<this->rows; r++) {
                    newMat->at(this->rows - r - 1, this->cols - c - 1) = this->at(r,c);
                }
            }
        } else if(rt == MAT_ROTATE_270) {
            for(int c = 0; c<this->cols; c++) {
                for(int r = 0; r<this->rows; r++) {
                    newMat->at(this->rows - c - 1, r) = this->at(r,c);
                }
            }
        } else {
            return;
        }
        this->data = newMat->data;
        delete newMat;
    }

    enum mirror_type {
        MAT_MIRROR_NONE = 0,
        MAT_MIRROR_LR,
       // MAT_MIRROR_TD,
        MAT_MIRROR_CNT
    };

    void mirror(int mt) {
        Mat * newMat = new Mat(this->rows, this->cols, this->channels);
        if(mt == MAT_MIRROR_LR) {
            for(int c = 0; c<this->cols; c++) {
                for(int r = 0; r<this->rows; r++) {
                    newMat->at(r,this->cols - c - 1) = this->at(r,c);
                }
            }
/*        } else if(mt == MAT_MIRROR_TD) {
            for(int c = 0; c<this->cols; c++) {
                for(int r = 0; r<this->rows; r++) {
                    newMat->at(this->rows - r - 1,c) = this->at(r,c);
                }
            }
*/        } else {
            return;
        }
        this->data = newMat->data;
        delete newMat;
    }

    void scale() {
        Mat * newMat = new Mat(this->rows/2, this->cols/2, this->channels);
        for(int c = 0; c<this->cols/2; c++) {
            for(int r = 0; r<this->rows/2; r++) {

                uint32_t pr = 0;
                uint32_t pg = 0;
                uint32_t pb = 0;

                pr = getR(this->at(r*2,c*2));
                //pr += getR(this->at(r*2+1,c*2));
                //pr += getR(this->at(r*2,c*2+1));
                //pr += getR(this->at(r*2+1,c*2+1));
                //pr /= 4;
                pr &= 0xff;


                pg = getG(this->at(r*2,c*2));
                //pg += getG(this->at(r*2+1,c*2));
                //pg += getG(this->at(r*2,c*2+1));
                //pg += getG(this->at(r*2+1,c*2+1));
                //pg /= 4;
                pg &= 0xff;

                pb = getB(this->at(r*2,c*2));
                //pb += getB(this->at(r*2+1,c*2));
                //pb += getB(this->at(r*2,c*2+1));
                //pb += getB(this->at(r*2+1,c*2+1));
                //pb /= 4;
                pb &= 0xff;


                newMat->at(r,c) = (pr << 16) | (pg<<8) | (pb);
            }
        }
        this->data = newMat->data;
        this->channels = newMat->channels;
        this->cols = newMat->cols;
        this->rows = newMat->rows;
        delete newMat;
    }

};


class RLFIC {
public:
    Mat * image;

    std::vector<Mat *> regionSet;
    std::vector<Mat *> domainSet;
    std::vector<Mat *> domainSetScaled;

    /* [cxHigh]|[cxLow]|[cyHigh]|[cyLow]|[modif]|[offR_high]|[offR_low]|[offG_high]|[offG_low]|[offB_high]|[offB_low] */


    std::vector<unsigned char> rawCoefs;

    RLFIC(Mat * _image) {
        domainSize = 8;
        domainOffset = 2;

        image = _image;
    }

    RLFIC() {
        domainSize = 8;
        domainOffset = 2;

    }


    int domainSize;
    int domainOffset;



    void generateDomainSet(); //source
    void generateRegionSet(); //destination
    int findClosestDom(Mat * reg);
    float getDist(Mat * dom, Mat * reg);
    void scaleDomainSet();
    void genCoefs();
    std::vector<int> getColorOffset(Mat * domSc, Mat * reg);
    void decompress();
    void decompressIter();
};




#endif
