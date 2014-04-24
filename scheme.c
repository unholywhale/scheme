#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef enum {BOOLEAN, CHARACTER, FIXNUM, STRING} object_type;

typedef struct {
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
  } data;
} object;

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
  else {
    fprintf(stderr, "Bad input. Unexpected character %c", c);
    exit(1);
  }
  exit(1);
}

void write(object *obj) {
  char c;
  char *str;
  
  switch (obj->type) {
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
