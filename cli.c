#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <decompress.h>
#include <parser.h>

static void print_string(FILE *fout, char *str) {
  int len = strlen(str);
  int i;
  for (i=0; i<len; ++i) {
    if (str[i] == '"' || str[i] == '\\')
      fputc('\\', fout);
    fputc(str[i], fout);
  }
}

static void print_qualifiers(FILE *fout, struct mof_qualifier *qualifiers, uint32_t count, char *prefix) {
  uint32_t i;
  if (count > 0 || prefix) {
    fprintf(fout, "[");
    if (prefix) {
      fprintf(fout, "%s", prefix);
      if (count > 0)
        fprintf(fout, ", ");
    }
    for (i = 0; i < count; ++i) {
      switch (qualifiers[i].type) {
      case MOF_QUALIFIER_BOOLEAN:
        print_string(fout, qualifiers[i].name);
        if (!qualifiers[i].value.boolean)
          fprintf(fout, "(FALSE)");
        break;
      case MOF_QUALIFIER_SINT32:
        print_string(fout, qualifiers[i].name);
        fprintf(fout, "(%d)", qualifiers[i].value.sint32);
        break;
      case MOF_QUALIFIER_STRING:
        print_string(fout, qualifiers[i].name);
        fprintf(fout, "(\"");
        print_string(fout, qualifiers[i].value.string);
        fprintf(fout, "\")");
        break;
      default:
        fprintf(fout, "unknown");
        break;
      }
      if (qualifiers[i].toinstance || qualifiers[i].tosubclass || qualifiers[i].disableoverride || qualifiers[i].amended) {
        fprintf(fout, " :");
        if (qualifiers[i].toinstance)
          fprintf(fout, " ToInstance");
        if (qualifiers[i].tosubclass)
          fprintf(fout, " ToSubclass");
        if (qualifiers[i].disableoverride)
          fprintf(fout, " DisableOverride");
        if (qualifiers[i].amended)
          fprintf(fout, " Amended");
      }
      if (i != count-1)
        fprintf(fout, ", ");
    }
    fprintf(fout, "]");
  }
}

static void print_variable_type(FILE *fout, struct mof_variable *variable) {
  char *type = NULL;
  switch (variable->variable_type) {
  case MOF_VARIABLE_BASIC:
  case MOF_VARIABLE_BASIC_ARRAY:
    switch (variable->type.basic) {
    case MOF_BASIC_TYPE_STRING: type = "string"; break;
    case MOF_BASIC_TYPE_REAL64: type = "real64"; break;
    case MOF_BASIC_TYPE_REAL32: type = "real32"; break;
    case MOF_BASIC_TYPE_SINT32: type = "sint32"; break;
    case MOF_BASIC_TYPE_UINT32: type = "uint32"; break;
    case MOF_BASIC_TYPE_SINT16: type = "sint16"; break;
    case MOF_BASIC_TYPE_UINT16: type = "uint16"; break;
    case MOF_BASIC_TYPE_SINT64: type = "sint64"; break;
    case MOF_BASIC_TYPE_UINT64: type = "uint64"; break;
    case MOF_BASIC_TYPE_SINT8: type = "sint8"; break;
    case MOF_BASIC_TYPE_UINT8: type = "uint8"; break;
    case MOF_BASIC_TYPE_DATETIME: type = "datetime"; break;
    case MOF_BASIC_TYPE_CHAR16: type = "char16"; break;
    case MOF_BASIC_TYPE_BOOLEAN: type = "boolean"; break;
    default: break;
    }
    break;
  case MOF_VARIABLE_OBJECT:
  case MOF_VARIABLE_OBJECT_ARRAY:
    type = variable->type.object;
    break;
  default:
    break;
  }
  fprintf(fout, "%s", type ? type : "unknown");
}

static void print_variable(FILE *fout, struct mof_variable *variable, char *prefix) {
  if (variable->qualifiers_count > 0 || prefix) {
    print_qualifiers(fout, variable->qualifiers, variable->qualifiers_count, prefix);
    fprintf(fout, " ");
  }
  print_variable_type(fout, variable);
  fprintf(fout, " ");
  print_string(fout, variable->name);
  if (variable->variable_type == MOF_VARIABLE_BASIC_ARRAY || variable->variable_type == MOF_VARIABLE_OBJECT_ARRAY) {
    fprintf(fout, "[");
    if (variable->has_array_max)
      fprintf(fout, "%d", variable->array_max);
    fprintf(fout, "]");
  }
}

static void print_classes(FILE *fout, struct mof_class *classes, uint32_t count) {
  char *direction;
  uint32_t i, j, k;
  int print_namespace = 0;
  int print_classflags = 0;
  for (i = 0; i < count; ++i) {
    if (!classes[i].name)
      continue;
    if (classes[i].namespace && strcmp(classes[i].namespace, "root\\default") != 0)
      print_namespace = 1;
    if (classes[i].classflags)
      print_classflags = 1;
  }
  for (i = 0; i < count; ++i) {
    if (!classes[i].name)
      continue;
    if (print_namespace) {
      fprintf(fout, "#pragma namespace(\"");
      if (classes[i].namespace)
        print_string(fout, classes[i].namespace);
      else
        print_string(fout, "root\\default");
      fprintf(fout, "\")\n");
    }
    if (print_classflags) {
      fprintf(fout, "#pragma classflags(");
      if (classes[i].classflags == 1)
        fprintf(fout, "\"updateonly\"");
      else if (classes[i].classflags == 2)
        fprintf(fout, "\"createonly\"");
      else if (classes[i].classflags == 32)
        fprintf(fout, "\"safeupdate\"");
      else if (classes[i].classflags == 33)
        fprintf(fout, "\"updateonly\", \"safeupdate\"");
      else if (classes[i].classflags == 64)
        fprintf(fout, "\"forceupdate\"");
      else if (classes[i].classflags == 65)
        fprintf(fout, "\"updateonly\", \"forceupdate\"");
      else
        fprintf(fout, "%d", (int)classes[i].classflags);
      fprintf(fout, ")\n");
    }
    if (classes[i].qualifiers_count > 0) {
      print_qualifiers(fout, classes[i].qualifiers, classes[i].qualifiers_count, NULL);
      fprintf(fout, "\n");
    }
    fprintf(fout, "class ");
    print_string(fout, classes[i].name);
    fprintf(fout, " ");
    if (classes[i].superclassname) {
      fprintf(fout, ": ");
      print_string(fout, classes[i].superclassname);
      fprintf(fout, " ");
    }
    fprintf(fout, "{\n");
    for (j = 0; j < classes[i].variables_count; ++j) {
      fprintf(fout, "  ");
      print_variable(fout, &classes[i].variables[j], NULL);
      fprintf(fout, ";\n");
    }
    if (classes[i].variables_count && classes[i].methods_count)
      fprintf(fout, "\n");
    for (j = 0; j < classes[i].methods_count; ++j) {
      fprintf(fout, "  ");
      if (classes[i].methods[j].qualifiers_count > 0) {
        print_qualifiers(fout, classes[i].methods[j].qualifiers, classes[i].methods[j].qualifiers_count, NULL);
        fprintf(fout, " ");
      }
      if (classes[i].methods[j].return_value.variable_type)
        print_variable_type(fout, &classes[i].methods[j].return_value);
      else
        fprintf(fout, "void");
      fprintf(fout, " ");
      print_string(fout, classes[i].methods[j].name);
      fprintf(fout, "(");
      for (k = 0; k < classes[i].methods[j].parameters_count; ++k) {
        switch (classes[i].methods[j].parameters_direction[k]) {
        case MOF_PARAMETER_IN:
          direction = "in";
          break;
        case MOF_PARAMETER_OUT:
          direction = "out";
          break;
        case MOF_PARAMETER_IN_OUT:
          direction = "in, out";
          break;
        default:
          direction = NULL;
          break;
        }
        print_variable(fout, &classes[i].methods[j].parameters[k], direction);
        if (k != classes[i].methods[j].parameters_count-1)
          fprintf(fout, ", ");
      }
      fprintf(fout, ");\n");
    }
    fprintf(fout, "};\n");
    if (i != count-1)
      fprintf(fout, "\n");
  }
}

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