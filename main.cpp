
#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_map>

using namespace std;

// ---------------- TOKEN TYPES ----------------
enum TokenType { T_EOF, T_NUMBER, T_IDENT, T_STRING, T_KEYWORD, T_SYM };

struct Token {
    TokenType type;
    string text;
    int number;
    Token(TokenType t=T_EOF, string s="", int n=0):type(t),text(s),number(n){}
};

// ---------------- GLOBALS ----------------
string src;
size_t posi = 0;
vector<Token> tokens;
size_t idx = 0;

set<string> keywords = {
    "agr",        // if
    "likhoisko",  // print
    "jbtk",       // while
    "wapiskrdo",  // return
    "yaa"         // or
};

set<string> dataTypes = {
    "numberkliye",
    "pointkliye",
    "characterskliye"
};

unordered_map<string,int> vars;
ofstream outfile;

// ---------------- LEXER ----------------
bool isIdStart(char c){ return isalpha(c)||c=='_'; }
bool isIdChar(char c){ return isalnum(c)||c=='_'; }

void skipSpace(){
    while(posi < src.size() && isspace(src[posi])) posi++;
}

Token nextToken(){
    skipSpace();
    if(posi >= src.size()) return Token(T_EOF,"");

    char c = src[posi];

    // Number
    if(isdigit(c)){
        int val=0;
        while(posi<src.size() && isdigit(src[posi])){
            val = val*10 + (src[posi]-'0');
            posi++;
        }
        return Token(T_NUMBER,"",val);
    }

    // Identifier / keyword
    if(isIdStart(c)){
        string s;
        while(posi<src.size() && isIdChar(src[posi])){
            s.push_back(src[posi++]);
        }
        if(keywords.count(s) || dataTypes.count(s))
            return Token(T_KEYWORD,s);
        return Token(T_IDENT,s);
    }

    // String
    if(c=='"'){
        posi++;
        string s;
        while(posi<src.size() && src[posi]!='"') s.push_back(src[posi++]);
        posi++;
        return Token(T_STRING,s);
    }

    // Symbols (Single char like <, >, +, -, *, /)
    posi++;
    return Token(T_SYM,string(1,c));
}

void tokenize(){
    while(true){
        Token t = nextToken();
        tokens.push_back(t);
        if(t.type==T_EOF) break;
    }
}

Token peek(){ return tokens[idx]; }
Token get(){ return tokens[idx++]; }

void expect(string s){
    if(peek().text!=s){
        cout<<"Syntax Error: Expected "<<s<<" but found "<<peek().text<<"\n";
        exit(1);
    }
    get();
}

// ---------------- EXPRESSION PARSER (UPDATED) ----------------
int expr();

// 1. Lowest Level: Numbers, Vars, Parentheses
int factor(){
    Token t = get();
    if(t.type==T_NUMBER) return t.number;
    if(t.type==T_IDENT) return vars[t.text];
    if(t.text=="("){
        int v = expr();
        expect(")");
        return v;
    }
    return 0;
}

// 2. Multiplicative: *, /
int term(){
    int v = factor();
    while(peek().text=="*" || peek().text=="/"){
        string op = get().text;
        int r = factor();
        if(op=="*") v*=r;
        else if (r != 0) v/=r;
    }
    return v;
}

// 3. Additive: +, -
int additive_expr(){
    int v = term();
    while(peek().text=="+" || peek().text=="-"){
        string op = get().text;
        int r = term();
        if(op=="+") v+=r;
        else v-=r;
    }
    return v;
}

// 4. Comparison: <, > (NEW LOGIC ADDED HERE)
int comparison_expr(){
    int v = additive_expr();

    // Check if next token is < or >
    if(peek().text == "<"){
        get(); // consume <
        int r = additive_expr();
        return (v < r); // Returns 1 if true, 0 if false
    }
    if(peek().text == ">"){
        get(); // consume >
        int r = additive_expr();
        return (v > r);
    }

    return v;
}

// 5. Logical: yaa (OR)
int expr(){
    int v = comparison_expr(); // Now calls comparison instead of additive directly
    while(peek().text == "yaa"){
        get();
        int r = comparison_expr();
        v = (v || r);
    }
    return v;
}

// ---------------- STATEMENTS ----------------
void statement();

void skip_block(){
    expect("{");
    int depth = 1;
    while(depth > 0 && peek().type != T_EOF){
        if(peek().text == "{") depth++;
        if(peek().text == "}") depth--;
        get();
    }
}

void block(){
    expect("{");
    while(peek().text!="}"){
        statement();
    }
    expect("}");
}

void statement(){
    Token t = peek();

    // Declaration
    if(t.type==T_KEYWORD && dataTypes.count(t.text)){
        get();
        string name = get().text;
        int val = 0;
        if(peek().text=="="){
            get();
            val = expr();
        }
        vars[name]=val;
        expect("$");
        return;
    }

    // Print
    if(t.text=="likhoisko"){
        get();
        expect("(");
        if(peek().type==T_STRING){
            outfile<<get().text<<"\n";
        } else {
            outfile<<expr()<<"\n";
        }
        expect(")");
        expect("$");
        return;
    }

    // If
    if(t.text=="agr"){
        get();
        expect("(");
        int cond = expr();
        expect(")");
        if(cond) block();
        else skip_block();
        return;
    }

    // While (Logic Updated for Loop)
    if(t.text=="jbtk"){
        get();
        size_t condStartIdx = idx; // Loop start point
        expect("(");
        int cond = expr();
        expect(")");

        size_t blockStartIdx = idx; // Block start point

        if (!cond) {
            skip_block();
            return;
        }

        while(true){
            // Block run karo
            idx = blockStartIdx;
            block();

            // Condition dobara check karo
            idx = condStartIdx;
            expect("(");
            cond = expr();
            expect(")");

            if(!cond) {
                idx = blockStartIdx;
                skip_block(); // Agar false hui to loop se bahar
                break;
            }
        }
        return;
    }

    // Return
    if(t.text=="wapiskrdo"){
        get();
        int v = expr();
        outfile<<"RETURN: "<<v<<"\n";
        expect("$");
        exit(0);
    }

    // Assignment
    if(t.type==T_IDENT){
        string name = get().text;
        expect("=");
        vars[name] = expr();
        expect("$");
        return;
    }

    cout<<"Unknown statement: "<<t.text<<"\n";
    get();
}

// ---------------- MAIN ----------------
int main(){
    // Setup Dummy Input if not exists
    ifstream check("input.txt");
    if(!check){
        ofstream dummy("input.txt");
        dummy << "numberkliye x = 5$\n"
                 "numberkliye y = 3$\n"
                 "agr (x yaa y) {\n"
                 "    likhoisko(\"Condition is true\")$\n"
                 "}\n"
                 "jbtk (x < 10) {\n"
                 "    likhoisko(x)$\n"
                 "    x = x + 1$\n"
                 "}\n"
                 "wapiskrdo x$";
        dummy.close();
    }
    check.close();

    ifstream in("input.txt");
    src.assign((istreambuf_iterator<char>(in)),istreambuf_iterator<char>());

    // Clear tokens/vars for clean start
    tokens.clear();
    vars.clear();
    idx = 0;
    posi = 0;

    tokenize();
    outfile.open("output.txt");

    while(peek().type!=T_EOF){
        statement();
    }
    outfile.close();

    cout<<"Compilation finished. Check output.txt\n";
    return 0;
}
