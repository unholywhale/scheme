#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef enum {FIXNUM} object_type;

typedef struct {
  object_type type;
  union {
    struct {
      long value;
    } fixnum;
  } data;
} object;

object *alloc_object() {
  object *obj;
  
  obj = malloc(sizeof(object));
  if (obj == NULL) {
    fprintf(stderr, "Not enough memory");
    exit(1);
  }
  return obj;
}

object *make_fixnum(long value) {
  object *obj = alloc_object();
  obj->type = FIXNUM;
  obj->data.fixnum.value = value;
  return obj;
}

char is_delimeter(int c) {
  return isspace(c) || c == EOF ||
          c == '('  || c == ')' ||
          c == '"'  || c == ';';
}

void eat_whitespace(FILE *in) {
  int c;
  
  while ((c = getc(in)) != EOF) {
    if (isspace(c)) {
      continue;
    }
    else if (c == ';') {
      while (((c = getc(in)) != EOF) && (c != '\n'))
	;
      continue;
      
    }
    ungetc(c, in);
    break;
  }
}

int peek(FILE *in) {
  int c;
  c = getc(in);
  ungetc(c, in);
  return c;
}

void write(object *obj) {
  switch (obj->type) {
  case FIXNUM:
    printf("%ld", obj->data.fixnum.value);
    break;
  default:
    fprintf(stderr, "Unknown object type");
    exit(1);
  }
}

object *eval(object *exp) {
  return exp;
}


object *read(FILE *in) {
  int c;
  long num = 0;
  short sign = 1;
  
  eat_whitespace(in);

  c = getc(in);

  if (isdigit(c) || (c == '-' && isdigit(peek(in)))) {
    if (c == '-') {
      sign = -1;
    }
    else {
      ungetc(c, in);
    }
    while (isdigit(c = getc(in))) {
      num = (num * 10) + (c - '0');
    }
    num *= sign;
    if (is_delimeter(c)) {
      ungetc(c, in);
      return make_fixnum(num);
    }
    else {
      fprintf(stderr, "number not followed by delimeter");
      exit(1);
    }
  }
  else {
    fprintf(stderr, "Unexpected character %c", c);
    exit(1);
  }
  
}

int main() {
  while(1) {
    printf("> ");
    write(eval(read(stdin)));
    printf("\n");
  }
  return 0;
}
