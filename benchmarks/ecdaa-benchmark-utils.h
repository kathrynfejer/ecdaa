/******************************************************************************
 *
 * Copyright 2017-2018 Xaptum, Inc.
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#define BENCHMARK_ASSERT(cond) \
    do \
    { \
        if (!(cond)) { \
            printf("Condition \'%s\' failed\n\tin file: \'%s\'\n\tin function: \'%s\'\n\tat line: %d\n", #cond,__FILE__,  __func__, __LINE__); \
            printf("exiting"); \
            exit(1); \
        } \
    } while(0);

void benchmark_randomness(void *buf, size_t buflen)
{
    // No need to worry about threading in these benchmarks
    static FILE *file_ptr = NULL;
    if (NULL == file_ptr)
        file_ptr = fopen("/dev/urandom", "r");
    BENCHMARK_ASSERT(file_ptr != NULL);

    size_t read_ret = fread(buf, 1, buflen, file_ptr);
    BENCHMARK_ASSERT(read_ret == buflen);
}
