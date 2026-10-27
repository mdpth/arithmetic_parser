// Matilde Purina 251ADB141
// https://github.com/mdpth/arithmetic_parser
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>
// https://www.geeksforgeeks.org/c/dynamic-memory-allocation-in-c-using-malloc-calloc-free-and-realloc/
//for parsers https://www.geeksforgeeks.org/compiler-design/recursive-descent-parser/
//within parsers https://www.geeksforgeeks.org/c/operator-precedence-and-associativity-in-c/
// https://www.geeksforgeeks.org/c/read-write-structure-from-to-a-file-in-c/
//https://stackoverflow.com/questions/13913017/are-floating-point-operations-in-c-associative
//https://www.geeksforgeeks.org/c/c-program-to-handle-divide-by-zero-and-multiple-exceptions/
// https://stackoverflow.com/questions/54510469/catching-a-missing-operand-or-operator-in-stack-while-converting-to-postfix-from
//https://stackoverflow.com/questions/15394216/integer-power-in-c
//https://www.geeksforgeeks.org/c/strcmp-in-c/
//https://www.geeksforgeeks.org/c/sprintf-in-c/

// AI chatgpt https://chatgpt.com/share/6ae0dde7-561c-83ed-bba7-7ac15e688035 for outline & additional help in confusing/complex parts
// prompt: "general outline with some instructions, for: ....."

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

// output file name - <OUTDIR>/task1_<name>_<lastname>_<studentid>.txt.
#define FNAME "matilde"
#define LNAME "purina"
#define ID "251ADB141"

char *read_file(const char *path, long *length) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return NULL;
    }
    
    //buffer cap; handle large files without slowing down from repeated copying
    //https://www.geeksforgeeks.org/c/dynamic-memory-allocation-in-c-using-malloc-calloc-free-and-realloc/
    long max = 1000;
    long used = 0;
    char *text = malloc(max); //allocation
    if (text == NULL) {
        fclose(fp);
        return NULL;
    }

    //read char by char, reallocating if need
      int c = fgetc(fp);
      while (c != EOF) {
        if (used + 1 >= max) {//increase max
            max = max * 2;
            char *temp = realloc(text, max);
            if (temp == NULL) {
                free(text);
                fclose(fp);
                return NULL;
            }
            text = temp;
        }
        text[used] = (char)c;
        used++;
        c = fgetc(fp); //stored, next
    }
    text[used] = '\0';
    *length = used;
    fclose(fp);
    return text;
}

typedef struct {
    int isFloat;// 0=integer i, 1=float d; 64-bit
    long long i;// 
    double d;
} Value; //groups

static Value make_int(long long x) {
    Value v;
    v.isFloat = 0; //integer
    v.i = x;
    v.d = 0.0; //init
    return v;
}

static Value make_float(double x) {
    Value v;
    v.isFloat = 1;//float
    v.d = x;
    v.i = 0; //init
    return v;
}

static double convert_double(Value v) {
    if (v.isFloat) {
        return v.d;
    }
    return (double)v.i;//ex 5 to 5.0
}

static const char *src;//point 2 all input txt
static long pos;//pos advances as charas are read
static long line_end;
static long err;//0 if no error, pos if error exists

static Value parse_expr(void);//declare for primary

static void skip_spaces(void) { 
    while (pos < line_end && (src[pos] == ' ' || src[pos] == '\t')) { //skip any spaces or tabs
        pos++;
    }
}
//power   := primary [ '**' power ]
//https://stackoverflow.com/questions/54510469/catching-a-missing-operand-or-operator-in-stack-while-converting-to-postfix-from
static Value parse_primary(void) {
    skip_spaces();

    if (pos >= line_end) {
        err = line_end + 1; //error after end
        return make_int(0); //failed
    }

    //check inside brackets
    if (src[pos] == '(') {
        pos++;
        Value value = parse_expr(); //save results in value
        if (err != 0) {
            return make_int(0);
        }
        skip_spaces();

        //line ends before )
        if (pos >= line_end) {
            err = line_end + 1;
            return make_int(0);
        }

        //next not )
        if (src[pos] != ')') {
            err = pos + 1;
            return make_int(0);
        }
        pos++;
        return value;
    }

    //not nums
    if (src[pos] < '0' || src[pos] > '9') { 
        err = pos + 1;
        return make_int(0);
    }

    long long n = 0;
    double deci = 0.0;
    while (pos < line_end && src[pos] >= '0' && src[pos] <= '9') {
        //turn chars into numbers
        n = n * 10 + (src[pos] - '0');
        deci = deci * 10 + (src[pos] - '0');
        pos++;
    }

    //decimal pt and unit after > float
    if (pos < line_end && src[pos] == '.') { 
        if (pos + 1 >= line_end || src[pos + 1] < '0' || src[pos + 1] > '9') { //if deci dig afterwards
            err = pos + 1; //invalid dec pt pos
            return make_int(0); 
        }
        pos++;

        double scale = 1.0; //decimal places
        while (pos < line_end && src[pos] >= '0' && src[pos] <= '9') { //digs after, till non or end
            deci = deci * 10 + (src[pos] - '0'); //append
            scale = scale * 10;
            pos++;
        }
        return make_float(deci / scale); //accumulated digits div scale = double value
    }
    return make_int(n); //no decimal point, integer
}

//integral with a power
static long long int_exp(long long base, long long exponent)  {
    long long result = 1;
    while (exponent > 0) {
        if (exponent % 2 == 1) { //odd, multiply
            result = result * base;
        }
        exponent = exponent / 2; //halve so 2^8 -> (2^2)^4
        if (exponent > 0) {
            base = base * base;
        }
    }
    return result;
}
//power   := primary [ '**' power ]
//https://stackoverflow.com/questions/15394216/integer-power-in-c
static Value parse_power(void) {
    Value base = parse_primary();
    if (err != 0) {
        return make_int(0);
    }

    skip_spaces();

    if (pos + 1 < line_end && src[pos] == '*' && src[pos + 1] == '*') { //if next is **
        long new_pos = pos;
        pos += 2;
        skip_spaces();
        long exp_beg = pos;//starting exponent pos
        Value exponent = parse_power();// 2^2^2 -> 2^(2^2) -> 2^4
        if (err != 0) {
            return make_int(0);
        }
        //boyh integers
        if (!base.isFloat && !exponent.isFloat) { 
            if (exponent.i < 0) {
                err = exp_beg + 1;//negative exp error
                return make_int(0);
            }
            return make_int(int_exp(base.i, exponent.i));//integer power
        }

        double d = pow(convert_double(base), convert_double(exponent)); //convert integer to double
        if (!isfinite(d)) { //error
            err = new_pos + 1;
            return make_int(0);
        }
        return make_float(d);
    }
    return base;
}
//term    := power { ('*' | '/') power }
//https://www.geeksforgeeks.org/c/c-program-to-handle-divide-by-zero-and-multiple-exceptions/
static Value parse_term(void) {
    Value left = parse_power();//firs tleft 
    while (err == 0) {
        skip_spaces();
        if (pos >= line_end) {
            break;
        }
        char op = src[pos];
        if (op != '*' && op != '/') {//only * /
            break;
        }
        long new_pos = pos;
        pos++;
        skip_spaces();
        long right_start = pos;//second, right
        Value right = parse_power();
        if (err != 0) {
            return make_int(0);
        }
        if (op == '/' && ((!right.isFloat && right.i == 0) || (right.isFloat && right.d == 0.0))) {//error dividend 0
            err = right_start + 1;
            return make_int(0);
        }
        if (!left.isFloat && !right.isFloat) { //integer
            if (op == '*') {
                left = make_int(left.i * right.i);
            } else {
                left = make_int(left.i / right.i);
            }
        } else {//doiuble
            double a = convert_double(left);
            double b = convert_double(right);
            double r;
            if (op == '*') {
                r = a * b;
            } else {
                r = a / b;
            }
            if (!isfinite(r)) {
                err = new_pos + 1; //invalid
                return make_int(0);
            }
            left = make_float(r);
        }
    }
    return left; //answer stored in left
}

//expr    := term  { ('+' | '-') term }
static Value parse_expr(void) {
    Value left = parse_term();
    while (err == 0) {
        skip_spaces();
        if (pos >= line_end) {
            break;
        }
        char op = src[pos];
        if (op != '+' && op != '-') {//only + -
            break;
        }
        long new_pos = pos;//in case
        pos++;
        Value right = parse_term();
        if (err != 0) {
            return make_int(0);
        }
        if (!left.isFloat && !right.isFloat) {//integer arithmetic
            if (op == '+') {
                left = make_int(left.i + right.i);
            } else {
                left = make_int(left.i - right.i);
            }
        } else {//double arithmetic
            double a = convert_double(left);
            double b = convert_double(right);
            double r;
            if (op == '+') {
                r = a + b;
            } else {
                r = a - b;
            }
            if (!isfinite(r)) {
                err = new_pos + 1;
                return make_int(0);
            }
            left = make_float(r);
        }
    }
    return left;//also stored in left
}

Value eval_line(const char *text, long start, long end, long *error_pos) {
    //setters
    src = text;
    pos = start;
    line_end = end;
    err = 0;

    Value result = parse_expr();
    if (err == 0) {//leftover charas/other errors 
        skip_spaces();
        if (pos < line_end) {
            err = pos + 1;
        }
    }
    *error_pos = err;
    return result;
}
//file hi.txt -> hi.txt, file hi -> hi, file hi. -> hi
static void get_basename(const char *input, char *base) {

    char *slash = strrchr(input, '/');//find last slasg
    if (slash == NULL) {//no slash
        strcpy(base, input);
    } else {
        strcpy(base, slash + 1);//after
    }
    char *dot = strrchr(base, '.');//find last dot
    if (dot != NULL) {
        *dot = '\0';//delete everything after
    }
}
//folder name
static void get_dir_name(const char *dir, char *name) {
    char copy[500];
    strncpy(copy, dir, sizeof(copy) - 1);//dir into copy
    copy[sizeof(copy) - 1] = '\0';
    long len = (long)strlen(copy);
    while (len > 1 && copy[len - 1] == '/') { //for path, until /
        copy[len - 1] = '\0';//remove /
        len--;//adjust
    }
    char *slash = strrchr(copy, '/');//start of folder name
    if (slash == NULL || len == 1) {
        strcpy(name, copy);
    } else {
        strcpy(name, slash + 1);//after /
    }
}
//outdir output folder
static int make_outdir(const char *outdir) {
    if (mkdir(outdir, 0700) != 0 && errno != EEXIST) {
        perror("ERROR: Output folder writing error.");
        return 1;
    }
    return 0;
}

//https://www.geeksforgeeks.org/c/read-write-structure-from-to-a-file-in-c/
//eval input file . write output into outdir
static int eval_file(const char *input, const char *outdir) {
    long length = 0;
    char *text = read_file(input, &length);
    if (text == NULL) {
        perror("ERROR: Input file reading error.");
        return 1;
    }

    char base[200];
    get_basename(input, base);//no path/extension

    if (make_outdir(outdir) != 0) {
        free(text);
        return 1;
    }

    //build output file
    char path[1000];
    sprintf(path, "%s/%s_%s_%s_%s.txt", outdir, base, FNAME, LNAME, ID);
    FILE *out = fopen(path, "w");
    if (out == NULL) {
        perror("Output file writing error.");
        free(text);
        return 1;
    }

    //check each ;line
    long start = 0;
    while (start < length) {
        long end = start;
        while (end < length && text[end] != '\n') {
            end++;
        }

        //spaces/tabs
        int blank = 1;
        for (long i = start; i < end; i++) {
            if (text[i] != ' ' && text[i] != '\t') {
                blank = 0;
            }
        }
        // skip # comment - first in line
        if (blank == 0) {
            long first = start;
            while (first < end && (text[first] == ' ' || text[first] == '\t')) {
                first++; //skip allt abs/spaces
            }
            if (text[first] == '#') {
                blank = 1;//skip
            }
        }

        if (blank == 0) {
            long error_pos = 0;//parsing pos error if
            Value result = eval_line(text, start, end, &error_pos);
            if (error_pos != 0) { //error, stop
                fclose(out);
                out = fopen(path, "w");//empty
                if (out == NULL) {
                    perror("Output file writing error.");
                    free(text);
                    return 1;
                }
                fprintf(out, "ERROR:%ld\n", error_pos);
                break;
            }
            // integers print with %lld, floats with %.15g.
            if (result.isFloat) {
                fprintf(out, "%.15g\n", result.d);
            } else {
                fprintf(out, "%lld\n", result.i);
            }
        }
        start = end + 1;//newline
    }

    fclose(out);
    free(text);
    return 0;
}

//https://www.geeksforgeeks.org/c/strcmp-in-c/ and https://www.geeksforgeeks.org/c/sprintf-in-c/ 
int main(int argc, char *argv[]) {
    const char *outdir = NULL;
    const char *input = NULL;
    const char *dir = NULL;

    //checka rguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) { //output folder name follows 
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: %s folder name needed.\n", argv[i]);
                return 1;
            }
            outdir = argv[i + 1];//store output folder name
            i++;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: %s folder name needed.\n", argv[i]);
                return 1;
            }
            dir = argv[i + 1]; //store input folder name
            i++;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "ERROR: not a valid option.%s\n", argv[i]);
            return 1;
        } else {
            if (input != NULL) {//input filename alr provided
                fprintf(stderr, "ERROR: only one input file is allowed.\n");
                return 1;
            }
            input = argv[i]; //store input filename
        }
    }

    if ((input == NULL && dir == NULL) || (input != NULL && dir != NULL)) {
        fprintf(stderr, "ERROR, USE: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n");
        return 1;
    }

    char default_outdir[500];
    if (outdir == NULL) {//use default name if nothing specified
        char name[200];
        if (dir != NULL) {
            get_dir_name(dir, name);
        } else {
            get_basename(input, name);
        }
        sprintf(default_outdir, "%s_%s_%s", name, FNAME, ID);//build
        outdir = default_outdir;
    }

    if (input != NULL) { //one file
        return eval_file(input, outdir);
    }

    //files in folder
    DIR *folder = opendir(dir); //open 2 read
    if (folder == NULL) {
        perror("ERROR: input folder couldn't be opened.");
        return 1;
    }
    if (make_outdir(outdir) != 0) {
        closedir(folder);
        return 1;
    }

    int status = 0;
    struct dirent *entry;
    while ((entry = readdir(folder)) != NULL) {
        long n = (long)strlen(entry->d_name); //length entry name
        if (n <= 4 || strcmp(entry->d_name + n - 4, ".txt") != 0) { //only .txt files, skip others
            continue;
        }
        char fullpath[1000];
        sprintf(fullpath, "%s/%s", dir, entry->d_name);//folder path, entry name
        struct stat info;
        if (stat(fullpath, &info) != 0 || !S_ISREG(info.st_mode)) { //entries cannot be checked/not regular files
            continue;
        }
        if (eval_file(fullpath, outdir) != 0) {
            status = 1; //file failed, continue others
        }
    }
    closedir(folder);
    return status;
}