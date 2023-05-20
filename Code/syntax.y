%{
#include "lex.yy.c"
#include "parse_tree.h"
void myerror(int errorLine,char *msg);
void yyerror(char *msg);
char _tmpstr[50];
/*void dfs_num(int x,int *tail){
    if(x>=10){
        dfs_num(x/10,tail);
    }
    _tmpstr[(*tail)++]=x%10+'0';
}
void //line_detail(int x){
    //printf("//line_detail %d\n",x);
    _tmpstr[0]='(';
    int tail=1;
    dfs_num(x,&tail);
    _tmpstr[tail]=')';
    _tmpstr[tail+1]=0;
}*/

%}

%token INT FLOAT ID
%token SEMI COMMA ASSIGNOP RELOP PLUS MINUS STAR DIV AND OR DOT NOT TYPE LP RP LB RB LC RC STRUCT RETURN IF ELSE WHILE

%nonassoc LOWEST

%nonassoc LTE 
%nonassoc ELSE

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT 
%left LP RP LB RB DOT

%%
Program:
    ExtDefList{
        @$.first_line=@1.first_line;
        ////line_detail(@$.first_line);
        $$=new_tree_node("Program",@$.first_line,1,$1);
        if(hasError==0){
            //print_tree($$);
            programParseTree=$$;
        }
    }
    ;
ExtDefList:
    ExtDef ExtDefList{
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("ExtDefList",@$.first_line,2,$1,$2);
    }
    |/*empty*/{
        $$=new_token_node("ExtDefList",-1,0,"empty");
    }
    ;
ExtDef:
    Specifier ExtDecList SEMI{
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("ExtDef",@$.first_line,3,$1,$2,$3);
    }
    |Specifier SEMI{
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("ExtDef",@$.first_line,2,$1,$2);
    }
    |Specifier FunDec CompSt{
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("ExtDef",@$.first_line,3,$1,$2,$3);
    }
    |Specifier FunDec SEMI{
        yyerror("function declaration is not allowed");
        yyerrok;
    }
    |Specifier error SEMI{
        myerror(@2.first_line,"Extern define error after specifier");
        yyerrok;
    }
    |error SEMI{
        myerror(@1.first_line,"Extern define without specifier");
        yyerrok;
    }
    ;
ExtDecList:
    VarDec{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("ExtDecList",@$.first_line,1,$1);
    }
    |VarDec COMMA ExtDecList{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("ExtDecList",@$.first_line,3,$1,$2,$3);
    }
    ;
//spec
Specifier:
    TYPE{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Specifier",@$.first_line,1,$1);
    }
    |StructSpecifier{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Specifier",@$.first_line,1,$1);
    }
    ;
StructSpecifier:
    STRUCT OptTag LC DefList RC{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("StructSpecifier",@$.first_line,5,$1,$2,$3,$4,$5);
    }
    |STRUCT Tag {

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("StructSpecifier",@$.first_line,2,$1,$2);
    }
    |STRUCT error RC{
        myerror(@2.first_line,"Struct error");
        yyerrok;
    }
    ;
OptTag:
    ID{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("OptTag",@$.first_line,1,$1);
    }
    |/*empty*/{
        $$=new_token_node("OptTag",-1,0,"empty");
    }
    ;
Tag:
    ID{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Tag",@$.first_line,1,$1);
    }
    ;
//dec
VarDec:
    ID{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("VarDec",@$.first_line,1,$1);
    }
    |VarDec LB INT RB{
        
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("VarDec",@$.first_line,4,$1,$2,$3,$4);
    }
    ;
FunDec:
    ID LP VarList RP{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("FunDec",@$.first_line,4,$1,$2,$3,$4);
    }
    |ID LP RP{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("FunDec",@$.first_line,3,$1,$2,$3);
    }
    |error RP{
        myerror(@1.first_line,"Function declare error");
        yyerrok;
    }
    ;
VarList:
    ParamDec COMMA VarList{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("VarList",@$.first_line,3,$1,$2,$3);
    }
    |ParamDec{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("VarList",@$.first_line,1,$1);
    }
    ;
ParamDec:
    Specifier VarDec{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("ParamDec",@$.first_line,2,$1,$2);
    };
//stmt
CompSt:
    LC DefList StmtList RC{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("CompSt",@$.first_line,4,$1,$2,$3,$4);
    }
    |error RC{
        myerror(@1.first_line,"Compst error");
        yyerrok;
    }
    ;
StmtList:
    Stmt StmtList{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("StmtList",@$.first_line,2,$1,$2);
    }
    |/*empty*/{
        $$=new_token_node("StmtList",-1,0,"empty");
    };
Stmt:
    Exp SEMI{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Stmt",@$.first_line,2,$1,$2);
    }
    |CompSt{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Stmt",@$.first_line,1,$1);
    }
    |RETURN Exp SEMI{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Stmt",@$.first_line,3,$1,$2,$3);
    }
    |IF LP Exp RP Stmt %prec LTE{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Stmt",@$.first_line,5,$1,$2,$3,$4,$5);
    }
    |IF LP Exp RP Stmt ELSE Stmt{
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Stmt",@$.first_line,7,$1,$2,$3,$4,$5,$6,$7);

    }
    |WHILE LP Exp RP Stmt{
        
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Stmt",@$.first_line,5,$1,$2,$3,$4,$5);
    }
    |error SEMI{
        myerror(@1.first_line,"Ordinary stmt error");
        yyerrok;
    }
    |WHILE error RP Stmt{
        myerror(@2.first_line,"While error");
        yyerrok;
    }
    |IF error RP Stmt ELSE Stmt{
        myerror(@2.first_line,"Close if error");

        yyerrok;
    }
    |IF error RP Stmt %prec LTE{
        myerror(@2.first_line,"Open if error");
        yyerrok;
    }
    ;
//local def
DefList:
    Def DefList{
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("DefList",@$.first_line,2,$1,$2);
    }
    |/*empty*/{
        $$=new_token_node("DefList",-1,0,"empty");
    }
    ;
Def:
    Specifier DecList SEMI{
         @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Def",@$.first_line,3,$1,$2,$3);
    }
    |Specifier error SEMI{
        myerror(@2.first_line,"Defination error after specifier");
        yyerrok;
    }
    ;
DecList:
    Dec{
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("DecList",@$.first_line,1,$1);
        }
    |Dec COMMA DecList{
                @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("DecList",@$.first_line,3,$1,$2,$3);
    }
    ;
Dec:
    VarDec{
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Dec",@$.first_line,1,$1);
    }
    |VarDec ASSIGNOP Exp{
        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Dec",@$.first_line,3,$1,$2,$3);
    }
    ;
Exp:
    Exp ASSIGNOP Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |Exp AND Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |Exp OR Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |Exp RELOP Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |Exp PLUS Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |Exp MINUS Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |Exp STAR Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |Exp DIV Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |LP Exp RP{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |MINUS Exp %prec NOT{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,2,$1,$2);
    }
    |NOT Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,2,$1,$2);
    }
    |ID LP Args RP{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,4,$1,$2,$3,$4);
    }
    |ID LP RP{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |Exp LB Exp RB{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,4,$1,$2,$3,$4);
    }
    |Exp DOT ID{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,3,$1,$2,$3);
    }
    |ID{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,1,$1);
    }
    |INT{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,1,$1);
    }
    |FLOAT{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Exp",@$.first_line,1,$1);
    }
    /*
    |error RP
    */
    ;
Args:
    Exp COMMA Args{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Args",@$.first_line,3,$1,$2,$3);
    }
    |Exp{

        @$.first_line=@1.first_line;
        //line_detail(@$.first_line);
        $$=new_tree_node("Args",@$.first_line,1,$1);
    }
    ;
%%
#define RED "/033[0;32;31m"
void yyerror(char *msg){
    printf("Error type B at Line %d: %s.\n",yylineno,msg);
    //printf("Error Type B at Line %d\n",yylineno);
    hasError=1;
}
void myerror(int errorLine,char *msg){
    //hasError=1;
    //printf("Error type B at Line %d: %s.\n",errorLine,msg);
}