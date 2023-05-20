%{    
#define YYSTYPE struct TreeNode*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yycolumn = 1;
int hasError = 0; 
struct TreeNode* programParseTree=NULL;
#define YY_USER_ACTION \
yylloc.first_line = yylloc.last_line = yylineno; \
yylloc.first_column = yycolumn; \
yylloc.last_column = yycolumn + yyleng - 1; \
yycolumn += yyleng;

#ifdef DEBUG
#define output(token) printf("%s\n",#token)
#else
#define output(token)\
if(strcmp(#token,"ID")==0||strcmp(#token,"TYPE")==0)yylval=new_token_node(#token,yylineno,1,yytext);\
else if (strcmp(#token,"INT")==0)yylval=new_int_node(#token,yylineno,atoi(yytext));\
else if (strcmp(#token,"FLOAT")==0)yylval=new_float_node(#token,yylineno,atof(yytext));\
else if (strcmp(#token,"RELOP")==0)yylval=new_relop_node(#token,yylineno,yytext);\
else yylval=new_token_node(#token,yylineno,0,"");\
return token

#endif


#include "syntax.tab.h"
#include "parse_tree.h"
%}

%option yylineno
digit [0-9]
IDstart [a-zA-Z_]
IDothers {digit}|{IDstart}

INT {digit}+
FLOAT ({digit}+\.{digit}+)
ID {IDstart}{IDothers}*
RELOP (>|<|<=|>=|==|!=)
TYPE (int|float)

%%
{INT} {
    output(INT);
}
{FLOAT} {
    output(FLOAT);
}
(\ |\t) {
#ifdef DEBUG
    printf("ws\n");
#endif
}
\n {
    yycolumn=1;
}
\/\/ {
    int c=-1;
    while(c!='\n'&&c!=0){
        c=input();
        //printf("//:%c %d\n",c,(int) c);
    }
    if(c=='\n'){
        yycolumn=1;
    }
}
\/\* {
    int c1=1,c2=1;
    while(c2!=EOF && c2!=0){
        if(c1=='*'&&c2=='/'){
            break;
        }
        c1=c2;
        c2=input();
        if(c2=='\n'){
            yycolumn=1;
        }
    }
    if(c1!='*'||c2!='/'){
        printf("Error Type A at line %d\n",yylineno);
    }
}

\; {
    output(SEMI);
}
\, {
    output(COMMA);
}
\= {
    output(ASSIGNOP);
}
{RELOP} {
    output(RELOP);
}
\+ {
    output(PLUS);
}
- {
    output(MINUS);
}
\* {
    output(STAR);
}
\/ {
    output(DIV);
}
&& {
    output(AND);
}
\|\| {
    output(OR);
}
\. {
    output(DOT);
}
! {
    output(NOT);
}
{TYPE} {
    output(TYPE);
}
\( {
    output(LP);
}
\) {
    output(RP);
}
\[ {
    output(LB);
}
\] {
    output(RB);
}
\{ {
    output(LC);
}
\} {
    output(RC);
}
struct {
    output(STRUCT);
}
return {
    output(RETURN);
}
if {
    output(IF);
}
else {
    output(ELSE);
}
while {
    output(WHILE);
}
{ID} {
    output(ID);
}
\r {
    //do nothing
}
. {
    printf("Error type A at Line %d: Mysterious characters \'%s\'.\n",yylineno, yytext);
    //printf("Error Type A at Line %d\n",yylineno);
    hasError=1;
}

%%
#ifdef DEBUG
int main(){
    yylex();
    return 0;
}
#endif