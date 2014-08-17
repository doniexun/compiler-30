/*-------------------------------*/
/*  Lexical Analysis  token_p.c  */
/*-------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
  Lparen, Rparen, Plus,  Minus,   Multi,  Divi,   Equal,    NotEq,
  Less,   LessEq, Great, GreatEq, SngQ,   DblQ,   Assign,   Semicolon,
  If,     Else,   Puts,  Ident,   IntNum,
  String, Letter, Digit, NulKind, EofTkn, Others, END_list,
} Kind;

#define ID_SIZ 31         /* Size of identifier */
#define TEXT_SIZ 100      /* Size of String */
typedef struct {
  Kind kind;              /* Kind of Token */
  char text[TEXT_SIZ+1];  /* String of Token */
  int  intVal;            /* Value(if a token is a constant) */
} Token;

void initChTyp(void);
Token nextTkn(FILE *fp);
Token set_kind(Token t);
int nextCh(FILE *fp);
void err_exit(char *s);
int is_ope2(int c1, int c2);

Kind ctyp[256]; // List of character's types
Token token;    // A token referred now

struct {
  char *ktext;
  Kind kkind;
} KeyWdTbl[] = {
  {"if",   If      }, {"else", Else     },
  {"puts", Puts    },
  {"(",    Lparen  }, {")",    Rparen   },
  {"+",    Plus    }, {"-",    Minus    },
  {"*",    Multi   }, {"/",    Divi     },
  {"==",   Equal   }, {"!=",   NotEq    },
  {"<",    Less    }, {"<=",   LessEq   },
  {">",    Great   }, {">=",   GreatEq  },
  {"=",    Assign  }, {";",    Semicolon},
  {"",     END_list},
};

int main(int argc, char *argv[])
{
  FILE *fp;
  char ch;

  if (argc == 1) exit(1);
  if ((fp=fopen(argv[1], "r")) == NULL) exit(1);
  
  initChTyp();
  printf("text      kind intVal\n");
  for (token = nextTkn(fp); token.kind != EofTkn; token = nextTkn(fp)) {
    printf("%-10s %3d %d\n", token.text, token.kind, token.intVal);
  }

  /*
  for (ch = nextCh(fp); ch != EOF; ch = nextCh(fp)) {
    printf("%d\n", ctyp[ch]);
  }
  printf("Lexical Analysis\n");
  */
  return 0;
}

/**
 * void initChTyp(void)
 * Initialize a list of character's type
 */
void initChTyp(void)
{
  int i;

  for (i = 0;   i <  256;  i++) { ctyp[i] = Others; }
  for (i = '0'; i <= '9';  i++) { ctyp[i] = Digit;  }
  for (i = 'A'; i <= 'Z';  i++) { ctyp[i] = Letter; }
  for (i = 'a'; i <= 'z';  i++) { ctyp[i] = Letter; }
  ctyp['_']  = Letter; ctyp['='] = Assign;
  ctyp['(']  = Lparen; ctyp[')'] = Rparen;
  ctyp['<']  = Less;   ctyp['>'] = Great;
  ctyp['+']  = Plus;   ctyp['-'] = Minus;
  ctyp['*']  = Multi;  ctyp['/'] = Divi;
  ctyp['\''] = SngQ;   ctyp['"'] = DblQ;
  ctyp[';']  = Semicolon;
}

/**
 * int nextCh(void)
 * A next character
 */
int nextCh(FILE *fp)
{
  static int c = 0;
  if (c == EOF) return c;
  if ((c=fgetc(fp)) == EOF) fclose(fp);
  return c;
}

/**
 * Token nextTkn(FILE *fp)
 * A next token
 */
Token nextTkn(FILE *fp)
{
  Token tkn = {NulKind, "", 0};
  int ct, num, errF = 0;
  char *p = tkn.text, *p_31 = p + ID_SIZ, *p_100 = p + TEXT_SIZ;
  static int ch = ' ';              /* static variable for saving previous ch */
  
  while (isspace(ch)) { ch = nextCh(fp); }  /* Skip over some spaces */
  if (ch == EOF) { tkn.kind = EofTkn; return tkn; }

  switch (ctyp[ch]) {
    case Letter:    /* Identifier */
      for ( ; ctyp[ch] == Letter || ctyp[ch] == Digit; ch = nextCh(fp)) {
        if (p < p_31) *p++ = ch;
      }
      *p = '\0';
      break;
    case Digit:     /* Number */
      for (num = 0; ctyp[ch] == Digit; ch = nextCh(fp)) {
        num = num*10 + (ch-'0');
      }
      tkn.kind = IntNum;
      tkn.intVal = num;   /* Store a value */
      break;
    case SngQ:      /* Character constant */
      ct = 0;
      for (ch = nextCh(fp); ch != EOF && ch != '\n' && ch != '\''; ch = nextCh(fp)) {
        if (++ct == 1) *p++ = tkn.intVal = ch; else errF = 1;
      }
      *p = '\0';
      if (ch == '\'') ch = nextCh(fp); else errF = 1;
      if (errF) err_exit("This character constant is invalid.");
      tkn.kind = IntNum;
      break;
    case DblQ:      /* String constant */
      for (ch = nextCh(fp); ch != EOF && ch != '\n' && ch != '"'; ch = nextCh(fp)) {
        if (p < p_100) *p++ = ch; else errF = 1;
      }
      *p = '\0';
      if (errF) err_exit("This string constant is too long.");
      if (ch != '"') err_exit("String constant should be closed '\'.");
      ch = nextCh(fp);
      tkn.kind = String;
      break;
    default:      /* Operator */
      *p++ = ch; ch = nextCh(fp);
      if (is_ope2(*(p-1), ch)) { *p++ = ch; ch = nextCh(fp); }
      *p = '\0';

  }
  
  /* If kind of token is still not set. */
  if (tkn.kind == NulKind) tkn = set_kind(tkn);
  if (tkn.kind == Others) {
    printf("Invalid token: (%s)\n", token.text); exit(1);
  }

  return tkn;
}

/**
 * Token set_kind(Token t)
 * Set kind of Token
 */
Token set_kind(Token t)
{
  int i;
  char *s = t.text;

  t.kind = Others;
  for (i = 0; KeyWdTbl[i].kkind != END_list; i++) {
    if (strcmp(s, KeyWdTbl[i].ktext) == 0) {
      t.kind = KeyWdTbl[i].kkind; return t;
    }
  }

  if (ctyp[*s] == Letter)   t.kind = Ident;
  else if (ctyp[*s] == Digit) t.kind = IntNum;
  return t;
}

/**
 * int is_ope2(int c1, int c2)
 * Check if operator is constructed from two characters
 */ 
int is_ope2(int c1, int c2)
{
  char s[] = "    ";
  s[1] = c1; s[2] = c2;
  return strstr(" <= >= == != ", s) != NULL;
}

/**
 * void err_exit(char *s)
 * End a program
 */
void err_exit(char *s)
{
  puts(s); exit(1);
}
