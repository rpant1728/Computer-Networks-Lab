# include <stdio.h>
# include <stdlib.h>

char get_enc_char(int num){
    if(num >=0 && num < 26) return 'A'+num;
    else if(num >= 26 && num < 52) return 'a'+num-26;
    else if(num >= 52 && num <= 61) return '0'+num-52;
    else if(num == 62) return '+';
    else if(num == 63) return '/';
    else {
        printf("Error!");
        return '~';
    }
}

int get_dec_char(char c){
    if(c >= 'A' && c <= 'Z') return c - 'A';
    else if(c >= 'a' && c <= 'z') return c - 'a' + 26;
    else if(c >= '0' && c <= '9') return c - '0' + 52;
    else if(c == '+') return 62;
    else if(c == '/') return 63;
    else {
        printf("Error!");
        return -1;
    }
}

char *encode(char *input, char *output){
    int i=0, len, k=0;
    while(*(input+i) != '\0'){
        i++;
    }
    len = i;
    int code;
    for(i=0; i<len; i+=3){
        int temp = 0;
        temp = temp | input[i];
        code = (temp >> 2) & 63;
        output[k++] = get_enc_char(code);
        if(i+1 < len){
            temp = temp << 8;
            temp = temp | input[i+1];
            code = (temp >> 4) & 63;
            output[k++] = get_enc_char(code);
        }
        else{
            code = (temp << 4) & 63;
            output[k++] = get_enc_char(code);
            output[k++] = '=';
            output[k++] = '=';
            break;
        }
        if(i+2 < len){
            temp = temp << 8;
            temp = temp | input[i+2];
            code = (temp >> 6) & 63;
            output[k++] = get_enc_char(code);
            code = temp & 63;
            output[k++] = get_enc_char(code);
        }
        else{
            code = (temp << 2) & 63;
            output[k++] = get_enc_char(code);
            output[k++] = '=';
            break;
        }
    }
    output[k] = '\0';
    return output;
}

char *decode(char *input, char *output){
    int i=0, len, k=0;
    while(*(input+i) != '\0'){
        i++;
    }
    len = i;
    for(i=0; i<len; i+=4){
        int temp = 0;
        temp = temp | get_dec_char(input[i]);
        temp = temp << 6;
        temp = temp | get_dec_char(input[i+1]);
        if(input[i+2] == '='){
            output[k++] = (temp >> 4) & 255;
        }
        else if(input[i+3] == '='){
            output[k++] = (temp >> 4) & 255;
            temp = temp << 6;
            temp = temp | get_dec_char(input[i+2]);
            output[k++] = (temp >> 2) & 255;
        }
        else {
            output[k++] = (temp >> 4) & 255;
            temp = temp << 6;
            temp = temp | get_dec_char(input[i+2]);
            output[k++] = (temp >> 2) & 255;
            temp = temp << 6;
            temp = temp | get_dec_char(input[i+3]);
            output[k++] = temp & 255;
        }
    }
    output[k] = '\0';
    return output;
}