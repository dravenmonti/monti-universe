#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Vectors for country borders
struct pos {
  int x;
  int y;
  int exists;
};

// Convert two coordinates into a hash using xorshift
uint64_t hash(uint64_t x, uint64_t y) {
  x ^= x << 13 + y << 13;
  y ^= x >> 7 + y >> 7;
  x ^= x << 17 + y << 17;
  y ^= x << 13 + y << 13;
  x ^= x >> 7 + y >> 7;
  y ^= x << 17 + y << 17;
  return x;
}

// Convert integer to string, useful for parsing command line arguments
int to_int(char *buf) {
  int i = 0, x = 0;
  for (; buf[x] >= '0' && buf[x] <= '9'; x++) {
    i = i * 10 + buf[x] - '0';
  }
  return i;
}

// Parse command line and generate image
int main(int argc, char **argv) {
  if (argc < 6) {
    printf("%s [input geography] [input heightmap] [seed] "
           "[rounds] [save interval] [output file prefix]\n",
           argv[0]);
    return 1;
  }

  int seed = to_int(argv[3]), rounds = to_int(argv[4]), intv = to_int(argv[5]),
      n, width, height;

  unsigned char *dataIn = stbi_load(argv[2], &width, &height, &n, 4);
  unsigned char *geoIn = stbi_load(argv[1], &width, &height, &n, 4);

  unsigned char *dataOut = malloc(width * height * 4);
  memset(dataOut, 0, width * height * 4);

  unsigned char *dataImg = malloc(width * height * 4);

  // CLEAN THIS UP !!
  struct pos *colonies = malloc(width * height * sizeof(struct pos));

  colonies[0].exists = 1;
  colonies[0].x = 0;
  colonies[0].y = 0;

  int r2 = 0;
  for (;;) {
    if (dataIn[colonies[0].x * width * 4 + colonies[0].y * 4] == 160)
      break;
    colonies[0].x = (hash(colonies[0].x + r2, colonies[0].y + r2) % height);
    r2++;
    colonies[0].y = (hash(colonies[0].x + r2, colonies[0].y + r2) % width);
    r2++;
  }

  int colonyCount = 1;

  for (int r = 0; r < rounds; r++) {
    int colonyCountOld = colonyCount;
    for (int i = 0; i < colonyCountOld; i++) {
      struct pos colony = colonies[i];
      double x = colony.x / 3000.0, y = colony.y / 3000.0;

      double h = (hash(colony.x + r, colony.y + r) % 6700417) / 6700417.0;

      double h2 = dataIn[colony.x * width * 4 + colony.y * 4] / 128.0 - 1.0;

      if (colony.x > -1 && colony.x < width && colony.y > -1 &&
          colony.y < height) {
        int c = colony.x * width * 4 + colony.y * 4;
        dataOut[c] = 1;
      }

      if (fabs(h2 - 0.6) / 0.6 * 5.0 * (h > 0.6 ? 1.0 : 10.0) * (h + 0.5) <
          5.0) {
        for (int a = 0; a < 9; a++) {

          if (colonyCount > width * height) {
            break;
          }

          if (((double)colonyCount) / colonyCountOld > 1.01)
            break;

          int ex = colony.x + (a % 3) - 1;
          int ey = colony.y + (a / 3) - 1;
          int c = ex * width * 4 + ey * 4;
          if (!(ex > -1 && ex < width && ey > -1 && ey < height))
            continue;
          if (dataOut[c] == 1)
            continue;
          dataOut[c] = 1;
          colonies[colonyCount].exists = 1;
          colonies[colonyCount].x = ex;
          colonies[colonyCount].y = ey;
          colonyCount++;
        }
      }
    }
    printf("rendered %i / %i\n", r, rounds);

    if (r % intv != 0)
      continue;

    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        int c = i * width * 4 + j * 4;
        int isColony = dataOut[c];

        dataImg[c] = geoIn[c] / (isColony ? 1 : 10);
        dataImg[c + 1] = geoIn[c + 1] / (isColony ? 1 : 10);
        dataImg[c + 2] = geoIn[c + 2] / (isColony ? 1 : 10);
        dataImg[c + 3] = 255;
      }
    }

    char fname[1024];
    snprintf(fname, 1024, "%s_%i.png", argv[6], r / intv);

    stbi_write_png(fname, width, height, 4, dataImg, width * 4);
  }

  free(dataOut);
  free(dataImg);

  stbi_image_free(dataIn);
}