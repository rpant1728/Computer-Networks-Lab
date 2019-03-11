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

void char_to_bin(char c, int arr[], int j){
    int n = c, i = 0, curr;
    while(i < 8){
        arr[7-i+j] = n%2;
        n /= 2;
        i++;
    }
}

int bin_to_int(int arr[], int j){
    int num = 0, m = 1;
    for(int i=0; i<6; i++){
        num += arr[j+5-i]*m;
        m *= 2;
    }
    return num;
}

char *encode(char *input, char output[]){
    int i=0, len, k=0;
    int temp[24];
    len = i;
    for(i=0; i<len; i+=3){
        char_to_bin(*(input+i), temp, 0);
        output[k++] = get_enc_char(bin_to_int(temp, 0));
        if(i+1 < len){
            char_to_bin(*(input+i+1), temp, 8);
            output[k++] = get_enc_char(bin_to_int(temp, 6));
        }
        else{
            for(int j=8; j<12; j++) temp[j] = 0;
            output[k++] = get_enc_char(bin_to_int(temp, 6));
            output[k++] = '=';
            output[k++] = '=';
            break;
        }
        if(i+2 < len){
            char_to_bin(*(input+i+2), temp, 16);
            output[k++] = get_enc_char(bin_to_int(temp, 12));
            output[k++] = get_enc_char(bin_to_int(temp, 18));
        }
        else{
            for(int j=16; j<18; j++) temp[j] = 0;
            output[k++] = get_enc_char(bin_to_int(temp, 12));
            output[k++] = '=';
            break;
        }
    }
    int enc_len;
    if(len%3 == 0) enc_len = 4*len/3;
    else enc_len = (len/3+1)*4;

    return output;
    // for(int j=0; j<enc_len; j++){
    //     printf("%c", output[j]);
    // }
    // printf("\n");
}