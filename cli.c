#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <decompress.h>
#include <parser.h>
#include <printer.h>


int process_data(char *data, uint32_t size, FILE *fout) {
  struct mof_classes classes;
  classes = parse_bmf(data, size);
  print_classes(fout, classes.classes, classes.count);
  free_classes(classes.classes, classes.count);
  return 0;
}

int main(int argc, char *argv[]) {
  FILE *fin;
  FILE *fout;
  uint32_t *pin;
  char *pout;
  long sin;
  size_t lin;
  uint32_t lout;
  int ret;
  if ((argc != 1 && argc != 2 && argc != 3) || (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))) {
    fprintf(stderr, "Usage: %s [input_file [output_file]]\n", argv[0]);
    return 1;
  }
  if (argc >= 2) {
    fin = fopen(argv[1], "rb");
    if (!fin) {
      fprintf(stderr, "Cannot open input file %s: %s\n", argv[1], strerror(errno));
      return 1;
    }
  } else {
    fin = stdin;
  }
  if (fseek(fin, 0, SEEK_END) == 0) {
    sin = ftell(fin);
    if (sin < 0) {
      fprintf(stderr, "Cannot determinate size of input file %s: %s\n", argc >= 2 ? argv[1] : "(stdin)", strerror(errno));
      if (argc >= 2)
        fclose(fin);
      return 1;
    }
    ++sin;
    rewind(fin);
    if (sin > 0x800000) {
      fprintf(stderr, "Size of input file %s too large\n", argc >= 2 ? argv[1] : "(stdin)");
      if (argc >= 2)
        fclose(fin);
      return 1;
    }
  } else {
    sin = 0x800000;
  }
  pin = malloc(sin);
  if (!pin) {
    fprintf(stderr, "Cannot allocate memory for input file %s\n", argc >= 2 ? argv[1] : "(stdin)");
    if (argc >= 2)
      fclose(fin);
    return 1;
  }
  lin = fread(pin, 1, sin, fin);
  if (ferror(fin)) {
    fprintf(stderr, "Failed to read data from input file %s\n", argc >= 2 ? argv[1] : "(stdin)");
    free(pin);
    if (argc >= 2)
      fclose(fin);
    return 1;
  } else if (!feof(fin)) {
    fprintf(stderr, "Data too large in input file %s\n", argc >= 2 ? argv[1] : "(stdin)");
    free(pin);
    if (argc >= 2)
      fclose(fin);
    return 1;
  }
  if (argc >= 2)
    fclose(fin);
  if (lin <= 16 || pin[0] != 0x424D4F46 || pin[1] != 0x00000001 || pin[2] != (uint32_t)lin-16) {
    fprintf(stderr, "Invalid input\n");
    free(pin);
    return 1;
  }
  lout = pin[3];
  if (lout > 0x2000000) {
    fprintf(stderr, "Invalid input\n");
    free(pin);
    return 1;
  }
  pout = malloc(lout);
  if (!pout) {
    fprintf(stderr, "Cannot allocate memory for decompression\n");
    free(pin);
    return 1;
  }
  if (ds_dec((char *)pin+16, lin-16, pout, lout, 0) != (int)lout) {
    fprintf(stderr, "Decompress failed\n");
    free(pin);
    free(pout);
    return 1;
  }
  free(pin);
  if (argc >= 3) {
    fout = fopen(argv[2], "wb");
    if (!fout) {
      fprintf(stderr, "Cannot open output file %s: %s\n", argv[2], strerror(errno));
      free(pout);
      return 1;
    }
  } else {
    fout = stdout;
  }
  ret = process_data(pout, lout, fout);
  free(pout);
  if (argc >= 3)
    fclose(fout);
  return ret;
}