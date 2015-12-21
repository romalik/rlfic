#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "rlfic.hpp"
#include <string.h>
#include <time.h>

enum {
    MODE_COMPRESS,
    MODE_DECOMPRESS,
    MODE_GENERATE_MODEL
};

enum {
    ALG_AC,
    ALG_PPM
};

size_t loadFileToMem(std::string filename, std::vector<unsigned char> & dest) {
    size_t sz = 0;
    std::ifstream file(filename.c_str(), std::ios::binary | std::ios::ate);
    sz = file.tellg();
    file.seekg(0, std::ios::beg);

    if(!sz) {
        return 0;
    }

    dest.clear();
    dest.resize(sz);

    if (!file.read((char *)dest.data(), sz)) {
        return 0;
    } else {
        return sz;
    }
}

void writeToFile(std::string filename, std::vector<unsigned char> & src) {
    std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out);

    file.write((char *)src.data(), src.size());
    file.close();
}

void badcommand(char ** argv) {
    printf("Usage: %s {c|d|g} input output [ppm]\n", argv[0]);
    exit(1);
}

int main(int argc, char ** argv) {

    if(argc < 4) {
        badcommand(argv);
    }

    int mode = MODE_COMPRESS;
    int alg = ALG_AC;

    if(!strcmp(argv[1], "-c")) {
        mode = MODE_COMPRESS;
    } else if(!strcmp(argv[1], "-d")) {
        mode = MODE_DECOMPRESS;
    } else {
        badcommand(argv);
    }

    if(argc > 4) {
        if(!strcmp(argv[4], "ppm")) {
            alg = ALG_PPM;
        }
    }


    if(mode == MODE_COMPRESS) {
        RLFIC rlfic(Mat::imread(argv[2]));
        rlfic.generateDomainSet();
        printf("domain set size: %d\n", rlfic.domainSet.size());
        rlfic.generateRegionSet();
        printf("region set size: %d\n", rlfic.regionSet.size());
        rlfic.scaleDomainSet();
        printf("Generating coefs\n");
        rlfic.genCoefs();
        printf("Coefs size: %lu\n", rlfic.rawCoefs.size());
        writeToFile(argv[3], rlfic.rawCoefs);


    } else if(mode == MODE_DECOMPRESS) {
        RLFIC rlfic;
        rlfic.image = new Mat(512,512,3);
        loadFileToMem(argv[2], rlfic.rawCoefs);
        rlfic.decompress();
    }
    return 0;
}

