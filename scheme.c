#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef enum {BOOLEAN, CHARACTER, EMPTY_LIST, FIXNUM, PAIR, STRING} object_type;

typedef struct object {
  object_type type;
  union {
    struct {
      long value;
    } fixnum;
    struct {
      char value;
    } boolean;
    struct {
      char value;
    } character;
    struct {
      char *value;
    } string;
    struct {
      struct object *car;
      struct object *cdr;
    } pair;
  } data;
} object;

object *empty_list;
object *true;
object *false;

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

object *make_character(char value) {
  object *obj = alloc_object();
  obj->type = CHARACTER;
  obj->data.character.value = value;
  return obj;
}

object *make_string(char *value) {
  object *obj = alloc_object();
  obj->type = STRING;
  obj->data.string.value = malloc(strlen(value) + 1);
  if (obj->data.string.value == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }
  strcpy(obj->data.string.value, value);
  return obj;
}

object *cons(object *car, object *cdr) {
  object *obj = alloc_object();
  obj->type = PAIR;
  obj->data.pair.car = car;
  obj->data.pair.cdr = cdr;
  return obj;
}

object *car(object *obj) {
  return obj->data.pair.car;
}

object *cdr(object *obj) {
  return obj->data.pair.cdr;
}

char is_pair(object *obj) {
  return obj->type == PAIR;
}

char is_fixnum(object *obj) {
  return obj->type == FIXNUM;
}

char is_boolean(object *obj) {
  return obj->type == BOOLEAN;
}

char is_character(object *obj) {
  return obj->type == CHARACTER;
}

char is_string(object *obj) {
  return obj->type == STRING;
}

char is_false(object *obj) {
  return obj == false;
}

char is_true(object *obj) {
  return !is_false(obj);
}

char is_delimiter(int c) {
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

void peek_expected_delimiter(FILE *in) {
  if (!is_delimiter(peek(in))) {
    fprintf(stderr, "character not followed by delimiter");
    exit(1);
  }
}

void eat_expected_string(FILE *in, char *str) {
  int c;
  while (*str != '\0') {
    c = getc(in);
    if (c != *str) {
      fprintf(stderr, "Unexpected character %c\n", c);
      exit(1);
    }
    str++;      
  }
}

void init() {
  empty_list = alloc_object();
  empty_list->type = EMPTY_LIST;
  
  true = alloc_object();
  true->type = BOOLEAN;
  true->data.boolean.value = 1;

  false = alloc_object();
  false->type = BOOLEAN;
  false->data.boolean.value = 0;  
}

object *eval(object *exp) {
  return exp;
}

object *read_character(FILE *in) {
  int c;

  c = getc(in);
  switch (c) {
  case EOF:
    fprintf(stderr, "Unexpected ending of character");
    exit(1);
  case 'n':
    if (peek(in) == 'e') {
      eat_expected_string(in, "ewline");
      peek_expected_delimiter(in);
      return make_character('\n');
    }
    break;
  case 's':
    if (peek(in) == 'p') {
      eat_expected_string(in, "pace");
      peek_expected_delimiter(in);
      return make_character(' ');
    }
    break;    
  }
  peek_expected_delimiter(in);
  return make_character(c);
}

object *read(FILE *in);

object *read_pair(FILE *in) {
  int c;
  object *car_obj;
  object *cdr_obj;

  c = getc(in);
  if (c == ')') {
    return empty_list;
  }
  ungetc(c, in);
  car_obj = read(in);

  eat_whitespace(in);

  c = getc(in);
  if (c == '.') {
    c = peek(in);
    if(!is_delimiter(c)) {
      fprintf(stderr, "dot not followed by delimiter\n");
      exit(1);
    }
    cdr_obj = read(in);
    eat_whitespace(in);
    c = getc(in);
    if (c != ')') {
      fprintf(stderr, "No expected trailing )\n");
      exit(1);
    }
    return cons(car_obj, cdr_obj);
  }
  else {
    ungetc(c, in);
    cdr_obj = read_pair(in);
    return cons(car_obj, cdr_obj);
  }
}

object *read(FILE *in) {
  int c;
  int i;
  long num = 0;
#define MAX_BUFF 1000
  char str[MAX_BUFF];
  short sign = 1;
  
  eat_whitespace(in);

  c = getc(in);

  if (c == '#') {
    c = getc(in);
    switch (c) {
    case 't':
      return true;
    case 'f':
      return false;
    case '\\':
      return read_character(in);
    default:
      fprintf(stderr, "Unknown boolean value");
      exit(1);
    }
  }
  else if (isdigit(c) || (c == '-' && isdigit(peek(in)))) { /* fixnum */
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
    if (is_delimiter(c)) {
      ungetc(c, in);
      return make_fixnum(num);
    }
    else {
      fprintf(stderr, "number not followed by delimeter");
      exit(1);
    }
  }
  else if (c == '"') {
    i = 0;
    while ((c = getc(in)) != '"') {
      if (c == '\\') {
	c = getc(in);
	if (c == 'n') {
	  c = '\n';
	}
      }
      if (c == EOF) {
	fprintf(stderr, "Non-terminated string\n");
	exit(1);
      }
      if (i < MAX_BUFF - 1) {
	str[i++] = c;
      }
      else {
	fprintf(stderr, "String is too long\n");
	exit(1);
      }
    }
    str[i] = '\0';
    return make_string(str);
  }
  else if (c == '(') {
    return read_pair(in);
  }
  else {
    fprintf(stderr, "Bad input. Unexpected character %c", c);
    exit(1);
  }
  exit(1);
}

void write(object *obj);

void write_pair(object *obj) {
  object *car_obj;
  object *cdr_obj;

  car_obj = car(obj);
  cdr_obj = cdr(obj);
  write(car_obj);
  if (is_pair(cdr_obj)) {
    printf(" ");
    write_pair(cdr_obj);
  }
  else if (cdr_obj == empty_list) {
    return;
  }
  else {
    printf(" . ");
    write(cdr_obj);
  }
}

void write(object *obj) {
  char c;
  char *str;
  
  switch (obj->type) {
  case EMPTY_LIST:
    printf("()");
    break;
  case BOOLEAN:
    printf("#%c", is_false(obj) ? 'f' : 't');
    break;
  case FIXNUM:
    printf("%ld", obj->data.fixnum.value);
    break;
  case CHARACTER:
    c = obj->data.fixnum.value;
    switch (c) {
    case '\n':
      printf("#\\newline");
      break;
    case ' ':
      printf("#\\space");
      break;
    default:
      printf("#\\%c", c);
      break;
    }
    break;
  case STRING:
    str = obj->data.string.value;
    putchar('"');
    while (*str != '\0') {
      switch (*str) {
      case '\n':
	printf("\\n");
	break;
      case '\\':
	printf("\\\\");
	break;
      case '"':
	printf("\\\"");
	break;
      default:
	putchar(*str);
      }
      str++;
    }
    putchar('"');
    break;
  case PAIR:
    printf("(");

    write_pair(obj);
    printf(")");
  default:
    fprintf(stderr, "Unknown object type");
    exit(1);
  }
}

int main() {
  init();
  
  while(1) {
    printf("> ");
    write(eval(read(stdin)));
    printf("\n");
  }
  return 0;
}
