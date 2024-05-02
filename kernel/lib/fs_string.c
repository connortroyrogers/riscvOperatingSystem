#include <barelib.h>
#include <fs.h>

int32 fs_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

char* fs_strcpy(char* destination, const char* source) {
    char* ptr = destination;
    while (*source != '\0') {
        *ptr++ = *source++;
    }
    *ptr = '\0'; 
    return destination;
}

int fs_strlen(const char* str) {
    int length = 0;
    while (*str != '\0') {
        length++;
        str++;
    }
    return length;
}
            