#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

double str_to_double(const char *str) {
    if (str == NULL) {
        return 0.0;
    }

    char *endptr;
    double value;

    // 检查是否为十六进制
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        // 跳过 "0x" 或 "0X"
        value = strtod(str + 2, &endptr);
    } else {
        value = strtod(str, &endptr);
    }

    // 如果转换失败，返回 0.0
    if (*endptr != '\0') {
        return 0.0;
    }

    return value;
}

int main() {
    const char *test_strings[] = {"123.45", "0x1A.F", "abc", "0X10.8", "1000", NULL};

    for (const char **s = test_strings; *s; ++s) {
        printf("Converting \"%s\": %.2f\n", *s, str_to_double(*s));
    }

    return 0;
}