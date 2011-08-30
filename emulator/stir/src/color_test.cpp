#include <stdio.h>
#include <stdlib.h>
#include "color.h"
#include "lodepng.h"

int main(int argc, char **argv) {

    if (argc != 4) {
	fprintf(stderr, "usage: %s in.png out.png mse\n", argv[0]);
	return 1;
    }

    CIELab::initialize();

    std::vector<uint8_t> imageIn;
    std::vector<uint8_t> pngIn;
    LodePNG::loadFile(pngIn, argv[1]);
    LodePNG::Decoder decoder;
    decoder.decode(imageIn, pngIn);

    ColorReducer reducer;
    std::vector<uint8_t> imageOut;
    unsigned numBytes = decoder.getWidth() * decoder.getHeight() * 4;
    
    for (unsigned i = 0; i < numBytes; i += 4)
	reducer.add(RGB565(&imageIn[i]));

    double maxMSE = atof(argv[3]);
    reducer.reduce(maxMSE);
    printf("Reduced to max MSE of %g. Ended up with %d colors.\n", maxMSE, reducer.numColors());

    for (unsigned i = 0; i < numBytes; i += 4) {
	RGB565 n = reducer.nearest(RGB565(&imageIn[i]));
	imageOut.push_back(n.red());
	imageOut.push_back(n.green());
	imageOut.push_back(n.blue());
	imageOut.push_back(0xFF);
    }
    
    LodePNG::Encoder encoder;
    std::vector<uint8_t> pngOut;
    encoder.encode(pngOut, &imageOut[0], decoder.getWidth(), decoder.getHeight());
    LodePNG::saveFile(pngOut, argv[2]);
    decoder.decode(imageOut, pngOut);

    return 0;
}
