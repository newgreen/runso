#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    printf("runso server is running...\n");
    
    for (;;)
    {
        sleep(10);
    }
    
    return 0;
}
