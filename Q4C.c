#include <stdio.h>
#include <stdlib.h>

int main() {
    // FIX: Using the absolute path avoids $PATH manipulation [cite: 35, 36]
    system("/usr/bin/cal"); 
    return 0;
}
