//
//  main.c
//  Yandex_test_6
//
//  Created by Екатерина Колесникова on 06.04.15.
//  Copyright (c) 2015 Kolesnikova Ekaterina. All rights reserved.
//

#include <stdio.h>

/*
 Напишите реализацию для функции, которая принимает на вход последовательность ASCII-символов и выдаёт самый часто повторяющийся символ:

 char mostFrequentCharacter(char* str, int size);

 Функция должна быть оптимизирована для выполнения на устройстве с двухядерным ARM-процессором и бесконечным количеством памяти.

 Поясните своё решение.
 */

/*
 Проходим по всему массиву и считаем количество вхождений каждого символа.
 Далее циклом по массиву длиной 256 находим среди них максимум.

 Для оптимизации выполнения на устройстве с двухядерным ARM-процессором необходимо разделить пробег по строке на 2 потока.
 Но для коротких строк затраты на создание потока будут дороже быстренького пробега по строке.
 Поэтому нужно определить максимальный размер строки, при котором деление на потоки является нецелесообразным.
 Это число выбирается после тестирования, а пока возмем: __MAX_SIZE_OF_NO_THREADS = 1Гб = 1024 * 8 = 8192.
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

#define __NUMBER_OF_ASCII 256
#define __THREADS_COUNT 2
#define __MAX_SIZE_OF_NO_THREADS 8192

char mostFrequentCharacter(char* str, int size);

int main(int argc, const char * argv[]) {

    char *str = "qwertyqweryqqqq";
    char mostFrequentChar = mostFrequentCharacter (str, (int)strlen(str));
    assert(mostFrequentChar == 'u');
    return 0;
}

struct ArgumentStruct {
    char *str;
    size_t size;
};

void *mostFrequentCharacterInThread (char *str, int size) {
    if (!str || size <= 0) {
        return '\0';
    }
    int *counts = (int *) malloc((size_t) __NUMBER_OF_ASCII * (size_t) sizeof(int));
    for (size_t i = 0; i < size; i++) {
        counts[str[i]]++;
    }
    return (void *)counts;
}

void *mostFrequentCharacterInThreadWithOneArg(void *arg) {
    struct ArgumentStruct argStruct = *((struct ArgumentStruct *) arg);
    return mostFrequentCharacterInThread(argStruct.str, (int)argStruct.size);
}

char mostFrequentCharacter (char *str, int size) {
    //В случае ввода некорректных данных (если строка пустая или задана отрицательная длина строки) - возвращаем символ '\0' .
    if (!str || size <= 0) {
        return '\0';
    }

    // Если длина строки меньше 2 - сразу возвращаем первый символ.
    if (size < 2) {
        return str[0];
    }

    char mostFrequent = 0;
    // Для строк длины < __MAX_SIZE_OF_NO_THREADS - простой цикл по строке.
    if (size < __MAX_SIZE_OF_NO_THREADS) {
        int *counts = (int *) malloc((size_t) __NUMBER_OF_ASCII * (size_t) sizeof(int));
        int maxCount = 0;

        for(size_t i = 0; i < __NUMBER_OF_ASCII; i++)
            counts[i] = 0;

        for (size_t i = 0; i < size; i++)
        {
            counts[str[i]]++;

            if(counts[str[i]] > maxCount) {
                maxCount = counts[str[i]];
                mostFrequent = str[i];
            }
        }
    } else { // Для строк длины > __MAX_SIZE_OF_NO_THREADS - в двух потоках проходим по половинам строки.
        pthread_t threads[__THREADS_COUNT];

        struct ArgumentStruct *args;
        args = (struct ArgumentStruct *)malloc(__THREADS_COUNT * sizeof(struct ArgumentStruct));

        size_t firstStrSize = size / __THREADS_COUNT;
        size_t secondStrSize = size - firstStrSize;

        args[0].size =  firstStrSize;
        args[0].str = str;

        args[1].size =  secondStrSize;
        args[1].str = str + firstStrSize;

        for (size_t i = 0; i < __THREADS_COUNT; ++i) {
            int errCode = pthread_create(&threads[i], NULL, (void *)mostFrequentCharacterInThreadWithOneArg, &args[i]);
            if (errCode) {
                fprintf(stderr,"Error creating thread %d\n", errCode);
                return 0;
            }
        }

        int **resultCounts;
        resultCounts = malloc(__THREADS_COUNT * sizeof(int *));

        for (size_t i = 0; i < __THREADS_COUNT; ++i) {
            int errCode = pthread_join(threads[i], (void *)&resultCounts[i]);
            if (errCode) {
                fprintf(stderr,"Error joining thread %d\n", errCode);
                return 0;
            }
            if (!resultCounts[i]) {
                fprintf(stderr, "Error on thread %zu\n", i);
                return 0;
            }
        }

        int maxCount = 0;
        for (size_t i = 0; i < __NUMBER_OF_ASCII; i++)
        {
            int currentCount = 0;
            currentCount += resultCounts[0][i];
            currentCount += resultCounts[1][i];

            if(currentCount > maxCount) {
                maxCount = currentCount;
                mostFrequent = (char)i;
            }
        }

        free(args);
        free(resultCounts);
        free (resultCounts[0]);
        free (resultCounts[1]);
    }
    
    return mostFrequent;
}
