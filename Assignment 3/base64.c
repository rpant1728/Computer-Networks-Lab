# include <stdio.h>
# include <stdlib.h>

// Maps values in the range 0-63 to a BASE64 character
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

// Maps a BASE64 character to values in the range 0-63
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

// Function to encode input ASCII string into BASE64 
char *encode(char *input, char *output){
    int i=0, len, k=0;
    while(*(input+i) != '\0'){          		// Get length of input string
        i++;
    }
    len = i;
    int code;
    for(i=0; i<len; i+=3){             			// Processing input string in chunks of 24 bits
        int temp = 0;
        temp = temp | input[i];
        code = (temp >> 2) & 63;			    // Get first chunk of 6 bits
        output[k++] = get_enc_char(code);		// Store the corresponding encoded BASE64 character
        if(i+1 < len){					        // If not end of input string
            temp = temp << 8;				    // Shift current input by 8 bits and get the next chunk of 8 bits
            temp = temp | input[i+1];
            code = (temp >> 4) & 63;			// Get the second chunk of 6 bits and store corresponding BASE64 character
            output[k++] = get_enc_char(code);
        }
        else{						            // If end of input string
            code = (temp << 4) & 63;			// Pad string with 4 zero bits
            output[k++] = get_enc_char(code);	// Store the corresponding encoded BASE64 character and append two "=" characters
            output[k++] = '=';
            output[k++] = '=';
            break;
        }
        if(i+2 < len){					        // If not end of input string
            temp = temp << 8;	
            temp = temp | input[i+2];			// Shift current input by 8 bits and get the next chunk of 8 bits	
            code = (temp >> 6) & 63;			// Get the third chunk of 6 bits and store corresponding BASE64 character
            output[k++] = get_enc_char(code);		
            code = temp & 63;				    // Get the fourth chunk of 6 bits and store corresponding BASE64 character
            output[k++] = get_enc_char(code);
        }
        else{						            // If end of input string
            code = (temp << 2) & 63;			// Pad string with 2 zero bits
            output[k++] = get_enc_char(code);	// Store the corresponding encoded BASE64 character and append one "=" characters
            output[k++] = '=';
            break;
        }
    }
    output[k] = '\0';
    return output;   
}

// Function to decode input BASE64 string to ASCII
char *decode(char *input, char *output){
    int i=0, len, k=0;
    while(*(input+i) != '\0'){ 				        // Get length of input string
        i++;
    }
    len = i;
    for(i=0; i<len; i+=4){				            // Processing input in chunks of 4 BASE64 encoded characters i.e 24 bits
        int temp = 0;
        temp = temp | get_dec_char(input[i]);		// Get the first two chunks of 6 bits
        temp = temp << 6;
        temp = temp | get_dec_char(input[i+1]);		
        if(input[i+2] == '='){				        // If decoded string will be of length 3k+1 and last 24 bits are being processed
            output[k++] = (temp >> 4) & 255;		// Store the last decoded ASCII character (8 bits)
        } 
        else if(input[i+3] == '='){			        // If decoded string will be of length 3k+2 and last 24 bits are being processed
            output[k++] = (temp >> 4) & 255;		// Store the first decoded ASCII character
            temp = temp << 6;				        // Get third chunk of 6 bits
            temp = temp | get_dec_char(input[i+2]);	
            output[k++] = (temp >> 2) & 255;		// Store the second decoded ASCII character
        }
        else {						                // If decoded string is of length 3k, or last 24 bits are not being processed
            output[k++] = (temp >> 4) & 255;		// Get consecutive 6 bits chunks and store corresponding ASCII characters
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