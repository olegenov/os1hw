#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 5000

void print_args() {
    printf("Incorrect args!\n");
    printf("Usable command: ./binary_extractor <input_file> <output_file>\n");
}

int is_binary_number(const char *str, int len) {
    for (int i = 0; i < len; i++) {
        if (str[i] != '0' && str[i] != '1') {
            return 0;
        }
    }

    return 1;
}

void extract_binary_numbers(char *input, char *output) {
    size_t len = strlen(input);

    int j = 0;
    int number = 0;
    char temp_number[BUFFER_SIZE] = {0};
    int temp_index = 0;

    for (int i = 0; i <= len; i++) {
        if (isdigit(input[i])) {
            if (!number) {
                number = 1;
                temp_index = 0;
            }

            temp_number[temp_index++] = input[i];
        } else {
            if (number) {
                if (is_binary_number(temp_number, temp_index)) {
                    if (j > 0) {
                        output[j++] = '&';
                    }
                    memcpy(&output[j], temp_number, temp_index);
                    j += temp_index;
                }
                number = 0;
            }
            memset(temp_number, 0, sizeof(temp_number));
        }
    }
    output[j] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_args();
        return 1;
    }

    int input_file = open(argv[1], O_RDONLY);
    if (input_file < 0) {
        perror("Failed to open input file");
        return 1;
    }

    int output_file = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_file < 0) {
        perror("Failed to open output file");
        close(input_file);
        return 1;
    }

    int pipe1[2], pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Failed to create pipes");
        close(input_file);
        close(output_file);
        return 1;
    }

    // First
    pid_t pid1 = fork();

    if (pid1 == 0) {
        close(pipe1[0]);
        char buffer[BUFFER_SIZE] = {0};

        ssize_t bytes_read = read(input_file, buffer, BUFFER_SIZE);

        if (bytes_read < 0) {
            perror("Failed to read from input file");
            close(input_file);
            exit(1);
        }

        write(pipe1[1], buffer, bytes_read);
        close(pipe1[1]);
        close(input_file);
        exit(0);
    }

    // Second
    pid_t pid2 = fork();

    if (pid2 == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        char buffer[BUFFER_SIZE] = {0};
        char result[BUFFER_SIZE] = {0};

        read(pipe1[0], buffer, BUFFER_SIZE);
        extract_binary_numbers(buffer, result);
        write(pipe2[1], result, strlen(result));
        close(pipe1[0]);
        close(pipe2[1]);
        exit(0);
    }

    // Third
    pid_t pid3 = fork();

    if (pid3 == 0) {
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[1]);

        char result[BUFFER_SIZE] = {0};
        read(pipe2[0], result, BUFFER_SIZE);
        write(output_file, result, strlen(result));
        close(pipe2[0]);
        close(output_file);
        exit(0);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);
    close(input_file);
    close(output_file);

    wait(NULL);
    wait(NULL);
    wait(NULL);

    return 0;
}
