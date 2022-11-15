#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define atoa(x)

char *string_copy(const char *from, char *to)
{
    for (char *p = to; (*p = *from) != '\0'; ++p, ++from)
    {
        ;
    }

    return to;
}
int get_rand_val()
{
    int lower = 0, upper = 1;
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}

int get_8_bit_bin_to_dec(char *bin)
{
    // printf("the bin: %s\n", bin);
    int p = 0, decimal = 0;
    
    for (int i = 0; i < strlen(bin); ++i)
        decimal = decimal*2 + (bin[i] - '0'); 
    return decimal;
}

char *get_32_bit_zeros()
{
    char zeros[32];
    for (int i = 0; i < 32; i++)
    {
        zeros[i] = '0';
    }
    char *zeros_ptr = zeros;
    return zeros_ptr;
}

char *get_8_bit_bin(int val)
{
    char bin[9];
    int pos = 0;
    for (int i = 7; i >= 0; i--)
    {
        bin[pos++] = (val & (1 << i)) ? '1' : '0';
    }
    char *bin_ptr = bin;
    return bin_ptr;
}

char *get_32_bit_bin(int val)
{
    char bin[32];
    int pos = 0;
    for (int i = 31; i >= 0; i--)
    {
        bin[pos++] = (val & (1 << i)) ? '1' : '0';
    }
    char *bin_ptr = bin;
    return bin_ptr;
}

char *dec_ip_to_bin_ip(char *dec_ip)
{
    printf("given ip: %s\n", dec_ip);
    char *words[4];
    char binary_ip[50] = "";

    char *token = strtok(dec_ip, ".");
    char *temp = get_8_bit_bin(atoi(token));
    printf("toke: %s bin: %s\n", token, temp);
    strcat(binary_ip, temp);

    int i = 1;
    while (token != NULL)
    {

        token = strtok(NULL, ".");
        if (token == NULL)
            break;
        temp = get_8_bit_bin(atoi(token));
        // printf("toke: %s bin: %s\n", token, temp);

        strcat(binary_ip, temp);
    }
    // printf("binary-ip: %s\n", binary_ip);
    char *binary_ip_ptr = binary_ip;
    return binary_ip_ptr;
}

char *bin_ip_to_dec_ip(char *bin_ip)
{
    int length = strlen(bin_ip);
    int c = 0;
    int position=0;
    char decimal_ip[30]="";
    int val_num = 1;
    while (position < length)
    {
        char sub[8];

        for (int i=0;i<8;i++){
            sub[i] = bin_ip[position+i];
        }
        int decimal_val = get_8_bit_bin_to_dec(sub);
        // printf("decimal -val: %d\n",decimal_val);
        
        char buf[5];
        sprintf( buf, "%d", decimal_val);
        strcat(decimal_ip, buf);
        if(val_num<4) strcat(decimal_ip, ".");
        val_num++;

        position = position+8;
    }
    char * dec_ip_ptr = decimal_ip;
    return dec_ip_ptr;
}
