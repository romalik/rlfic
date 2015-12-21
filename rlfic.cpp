#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <math.h>
#include "rlfic.hpp"




void RLFIC::generateDomainSet() {
    for(int c = 0; c<image->cols-domainSize; c+=domainOffset) {
        for(int r = 0; r<image->rows-domainSize; r+=domainOffset) {
            Mat * croppedImg = image->crop(r,c,domainSize, domainSize);
            croppedImg->cx = c;
            croppedImg->cy = r;
            domainSet.push_back(croppedImg);
        }
    }

}

void RLFIC::generateRegionSet() {
    for(int r = 0; r<image->rows; r+=(domainSize/2)) {
        for(int c = 0; c<image->cols; c+=(domainSize/2)) {
            Mat * croppedImg = image->crop(r,c,domainSize/2, domainSize/2);
            croppedImg->attrs = 0;
            regionSet.push_back(croppedImg);
        }
    }
}

int RLFIC::findClosestDom(Mat *reg) {
    float minDist = -1;
    int minIdx = -1;
    for(int i = 0; i<domainSetScaled.size();i++) {
        float cDist = getDist(reg, domainSetScaled[i]);
        if((cDist < minDist) || (minIdx < 0)) {
            minIdx = i;
            minDist = cDist;
        }
    }
    return minIdx;
}

void RLFIC::scaleDomainSet() {
    for(int i = 0; i<domainSet.size(); i++) {
        for(int rt = Mat::MAT_ROTATE_0; rt < Mat::MAT_ROTATE_CNT; rt++) {
            for(int mt = Mat::MAT_MIRROR_NONE; mt < Mat::MAT_MIRROR_CNT; mt++) {
                Mat *newMat = new Mat(*domainSet[i]);
                newMat->scale();
                newMat->rotate(rt);
                newMat->mirror(mt);
                newMat->attrs = ((mt&0x2) << 2)|(rt&0x3);
                domainSetScaled.push_back(newMat);
            }
        }
    }
}

float RLFIC::getDist(Mat * domScaled, Mat * reg) {
    int sdR = 0, sdB = 0, sdG = 0;
    int srR = 0, srB = 0, srG = 0;

    for(int c = 0; c<reg->cols; c++) {
        for(int r = 0; r<reg->rows; r++) {
            uint32_t dR, dG, dB, rR, rG, rB;
            uint32_t dC, rC;
            dC = domScaled->at(r,c);
            dR = Mat::getR(dC);
            dG = Mat::getG(dC);
            dB = Mat::getB(dC);

            rC = reg->at(r,c);
            rR = Mat::getR(rC);
            rG = Mat::getG(rC);
            rB = Mat::getB(rC);

            sdR += dR;
            srR += rR;

            sdG += dG;
            srG += rG;

            sdB += dB;
            srB += rB;

        }
    }

    float qR = (sdR - srR)/(reg->cols*reg->rows);
    float qG = (sdG - srG)/(reg->cols*reg->rows);
    float qB = (sdB - srB)/(reg->cols*reg->rows);

    float dist = 0;

    for(int c = 0; c<reg->cols; c++) {
        for(int r = 0; r<reg->rows; r++) {
            int dR, dG, dB, rR, rG, rB;
            uint32_t dC, rC;
            dC = domScaled->at(r,c);
            dR = Mat::getR(dC);
            dG = Mat::getG(dC);
            dB = Mat::getB(dC);

            rC = reg->at(r,c);
            rR = Mat::getR(rC);
            rG = Mat::getG(rC);
            rB = Mat::getB(rC);
            float eDistR = 0.75*rR + qR - dR;
            float eDistG = 0.75*rG + qG - dG;
            float eDistB = 0.75*rB + qB - dB;
            dist += eDistR*eDistR;
            dist += eDistG*eDistG;
            dist += eDistB*eDistB;

        }
    }

    return dist;
}

void RLFIC::genCoefs() {
    std::vector<int> idxs;
    time_t t1 = time(NULL);
    printf("timestamp start %lu\n", t1);

    for(int d = 0; d< regionSet.size(); d++) {
        idxs.push_back(findClosestDom(regionSet[d]));
    }
    time_t t2 = time(NULL);
    printf("timestamp end %lu\n", t2);
    printf("avTime: %f\nTotal time: %f\n", static_cast<float>((time(NULL) - t1))/static_cast<float>(idxs.size()),
           static_cast<float>((time(NULL) - t1))/static_cast<float>(idxs.size())*static_cast<float>(regionSet.size()));

    for(int i = 0; i<idxs.size(); i++) {
        Mat * dom = domainSetScaled[idxs[i]];
        Mat * reg = regionSet[i];

        int cx = dom->cx;
        int cy = dom->cy;
        char modif = dom->attrs;
        std::vector<int> cScale = getColorOffset(dom, reg);

        rawCoefs.push_back((cx&0xff00)>>8);
        rawCoefs.push_back((cx&0xff));
        rawCoefs.push_back((cy&0xff00)>>8);
        rawCoefs.push_back((cy&0xff));
        rawCoefs.push_back(modif);

        rawCoefs.push_back((cScale[0] & 0xff00) >> 8);
        rawCoefs.push_back((cScale[0] & 0xff));
        rawCoefs.push_back((cScale[1] & 0xff00) >> 8);
        rawCoefs.push_back((cScale[1] & 0xff));
        rawCoefs.push_back((cScale[2] & 0xff00) >> 8);
        rawCoefs.push_back((cScale[2] & 0xff));

    }


}

std::vector<int> RLFIC::getColorOffset(Mat *domSc, Mat *reg) {

    int sdR=0, sdG=0, sdB=0, srR=0, srG=0, srB=0;
    for(int c = 0; c<reg->cols; c++) {
        for(int r = 0; r<reg->rows; r++) {
            int dR, dG, dB, rR, rG, rB;
            uint32_t dC, rC;
            dC = domSc->at(r,c);
            dR = Mat::getR(dC);
            dG = Mat::getG(dC);
            dB = Mat::getB(dC);

            rC = reg->at(r,c);
            rR = Mat::getR(rC);
            rG = Mat::getG(rC);
            rB = Mat::getB(rC);

            sdR += dR;
            srR += rR;

            sdG += dG;
            srG += rG;

            sdB += dB;
            srB += rB;

        }
    }

    float qR = (sdR - srR)/(reg->cols*reg->rows);
    float qG = (sdG - srG)/(reg->cols*reg->rows);
    float qB = (sdB - srB)/(reg->cols*reg->rows);


    int tR, tG, tB;
    tR = static_cast<int>(qR);
    tG = static_cast<int>(qG);
    tB = static_cast<int>(qB);
/*
    if(tR > 127) tR = 127;
    if(tG > 127) tG = 127;
    if(tB > 127) tB = 127;
    if(tR < -127) tR = -127;
    if(tG < -127) tG = -127;
    if(tB < -127) tB = -127;
    uint32_t res = ((tR&0xff) << 16) | ((tG&0xff) << 8) | (tB&0xff);
*/
    std::vector<int> res;
    res.push_back(tR);
    res.push_back(tG);
    res.push_back(tB);

    return res;

}

void RLFIC::decompressIter() {
    Mat * newImage = new Mat(*image);
    for(int i = 0; i<rawCoefs.size(); i+=11) {
        int dx = (rawCoefs[i] << 8) | rawCoefs[i+1];
        int dy = (rawCoefs[i+2] << 8) | rawCoefs[i+3];
        char modif = rawCoefs[i+4];
        int offR = static_cast<int16_t>((rawCoefs[i+5] << 8) | rawCoefs[i+6]);
        int offG = static_cast<int16_t>((rawCoefs[i+7] << 8) | rawCoefs[i+8]);
        int offB = static_cast<int16_t>((rawCoefs[i+9] << 8) | rawCoefs[i+10]);

        int regsInLine = image->cols / (domainSize/2);
        int cReg = i/11;

        int cLine = cReg / regsInLine;
        int cCol = cReg % regsInLine;

        int ry = cLine * domainSize/2;
        int rx = cCol * domainSize/2;

        /*
        int ry = ((i/8) / ((image->cols - domainSize/2)/(domainSize/2))) * domainSize/2;
        int rx = ((i/8) % ((image->cols - domainSize/2)/(domainSize/2))) * domainSize/2;
        */
        printf("%d %d %d: Dec: dx %d dy %d modif %d offR %d offG %d offB %d ry %d rx %d\n", i/8, i, rawCoefs.size(),dx, dy, modif, offR, offG, offB, ry, rx);

        Mat * dom = image->crop(dy, dx, domainSize, domainSize);
        dom->scale();
        if(modif & 0x4) { //mirror first
            dom->mirror(Mat::MAT_MIRROR_LR);
        }
        if((modif & 0x3) == Mat::MAT_ROTATE_0) {
            //no rotate
        } else if((modif & 0x03) == Mat::MAT_ROTATE_90) {
            dom->rotate(Mat::MAT_ROTATE_90);
        } else if((modif & 0x03) == Mat::MAT_ROTATE_180) {
            dom->rotate(Mat::MAT_ROTATE_180);
        } else if((modif & 0x03) == Mat::MAT_ROTATE_270) {
            dom->rotate(Mat::MAT_ROTATE_270);
        }


        for(int r = 0; r<domainSize/2; r++) {
            for(int c = 0; c<domainSize/2; c++) {

                //ImageN(dy+r, dx+c) = (image(ry+r/2, rx+c/2) + (offR, offG, offB))/0.75;

                int dR = (static_cast<int>(Mat::getR(dom->at(r, c)))*0.75 - offR);
                int dG = (static_cast<int>(Mat::getG(dom->at(r, c)))*0.75 - offG);
                int dB = (static_cast<int>(Mat::getB(dom->at(r, c)))*0.75 - offB);

                if(dR > 255) dR = 255;
                if(dG > 255) dG = 255;
                if(dB > 255) dB = 255;
                if(dR < 0) dR = 0;
                if(dG < 0) dG = 0;
                if(dB < 0) dB = 0;


                newImage->at(ry+r,rx+c) = ((dR&0xff)<<16) | ((dG&0xff)<<8) | (dB&0xff);

            }
        }
    }
    delete image;
    image = newImage;
}

void RLFIC::decompress() {
    for(int i = 0; i<150; i++) {
        char buf[100];
        sprintf(buf,"iter_%d.bmp", i);
        image->imwrite(buf);
        decompressIter();
    }
}
