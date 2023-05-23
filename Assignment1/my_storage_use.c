#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main() {
    
    char usageInfo[4096];
    int usageTokenCount;
    char* usageToken;
    float usageCount = 0;
    float usageTotal = 0;
    float usageAverage = 0;

    // open usage file and output to usage_info.txt
    FILE *usageFile = popen("df -h", "r");
    FILE *usageOutput = fopen("usage_info.txt", "w");
    if (usageFile) {
        while(fgets(usageInfo, sizeof(usageInfo), usageFile)) {
            // output to usage_info.txt
            fputs(usageInfo, usageOutput);

            // get usage
            usageTokenCount = 0;
            usageToken = strtok(usageInfo, " ");
            while(usageTokenCount < 4) {
                usageToken = strtok(NULL, " ");
                usageTokenCount++;
            }
            
            // calculate usage average
            usageTotal += atoi(usageToken);
            usageCount++;
        }
    }

    // calculate usage average and print the result
    usageAverage = usageTotal / (usageCount - 1);
    printf("%.2f\n", usageAverage);

    pclose(usageFile);
    fclose(usageOutput);
    return 0;
}
