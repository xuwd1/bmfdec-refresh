#include <stdint.h>
#include <stdio.h>
#include <parser.h>

#ifndef PRINTER_H
#define PRINTER_H

void print_classes(FILE *fout, struct mof_class *classes, uint32_t count);

#endif