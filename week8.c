#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void convertToGrayscale(const char *bmpFilePath) {
    int bmpFile = open(bmpFilePath, O_RDWR);
    if (bmpFile == -1) {
        perror("Error opening BMP file");
        return;
    }

    uint8_t header[54];
    if (read(bmpFile, header, 54) != 54) {
        perror("Error reading BMP header");
        close(bmpFile);
        return;
    }

    if (header[0] != 'B' || header[1] != 'M') {
        perror("Not a valid BMP file");
        close(bmpFile);
        return;
    }

    int32_t width = *(int32_t*)&header[18];
    int32_t height = *(int32_t*)&header[22];
    uint32_t dataOffset = *(uint32_t*)&header[10];

    lseek(bmpFile, dataOffset, SEEK_SET);
    uint8_t pixel[3];
    int padding = (4 - (width * 3) % 4) % 4;
    uint8_t gray;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {

            if (read(bmpFile, pixel, 3) != 3) {
                perror("Error reading pixel data");
                close(bmpFile);
                return;
            }

            gray = (uint8_t)(0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]);

            pixel[0] = pixel[1] = pixel[2] = gray;
            lseek(bmpFile, -3, SEEK_CUR);
            if (write(bmpFile, pixel, 3) != 3) {
                perror("Error writing gray pixel");
                close(bmpFile);
                return;
            }
        }

        lseek(bmpFile, padding, SEEK_CUR);
    }

    close(bmpFile);
}

void writeStatistics(const char *inputPath, const char *outputPath, int isBmp, int isDir, int isLnk) {
    struct stat fileStat;
    if (lstat(inputPath, &fileStat) < 0) {
        perror("Error getting file stats");
        return;
    }

    int outputFile = open(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFile == -1) {
        perror("Error opening output file");
        return;
    }

    char buffer[2048];
    int length = 0;

    if (isBmp) {
        int bmpFile = open(inputPath, O_RDONLY);
        if (bmpFile == -1) {
            perror("Error opening BMP file");
            close(outputFile);
            return;
        }
        convertToGrayscale(inputPath);
        lseek(bmpFile, 18, SEEK_SET);
        int32_t width, height;
        read(bmpFile, &width, sizeof(width));
        read(bmpFile, &height, sizeof(height));
        close(bmpFile);

        length += sprintf(buffer + length,
            "nume fisier: %s\n"
            "inaltime: %d\n"
            "lungime: %d\n",
            inputPath, height, width);
    } else if (isDir) {
        length += sprintf(buffer + length,
            "nume director: %s\n",
            inputPath);
    } else if (isLnk) {
        length += sprintf(buffer + length,
            "nume legatura: %s\n",
            inputPath);
    } else {
        length += sprintf(buffer + length, "nume fisier: %s\n", inputPath);
    }

    length += sprintf(buffer + length,
        "dimensiune: %ld\n"
        "identificatorul utilizatorului: %d\n"
        "timpul ultimei modificari: %s"
        "contorul de legaturi: %ld\n"
        "drepturi de acces user: %c%c%c\n"
        "drepturi de acces grup: %c%c%c\n"
        "drepturi de acces altii: %c%c%c\n",
        fileStat.st_size,
        fileStat.st_uid,
        ctime(&fileStat.st_mtime),
        fileStat.st_nlink,
        (fileStat.st_mode & S_IRUSR) ? 'R' : '-',
        (fileStat.st_mode & S_IWUSR) ? 'W' : '-',
        (fileStat.st_mode & S_IXUSR) ? 'X' : '-',
        (fileStat.st_mode & S_IRGRP) ? 'R' : '-',
        (fileStat.st_mode & S_IWGRP) ? 'W' : '-',
        '-',
        (fileStat.st_mode & S_IROTH) ? 'R' : '-',
        (fileStat.st_mode & S_IWOTH) ? 'W' : '-',
        '-'
    );

    write(outputFile, buffer, length);
    close(outputFile);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        write(STDOUT_FILENO, "Usage: ./program <director_intrare> <director_iesire>\n", 57);
        return -1;
    }

    DIR *dir = opendir(argv[1]);
    if (dir == NULL) {
        perror("Error opening input directory");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("Error creating process");
            continue;
        }

        if (pid == 0) {
            char inputPath[1024];
            char outputPath[1024];
            sprintf(inputPath, "%s/%s", argv[1], entry->d_name);
            sprintf(outputPath, "%s/%s_statistica.txt", argv[2], entry->d_name);

            struct stat fileStat;
            if (lstat(inputPath, &fileStat) < 0) {
                perror("Error getting file stats");
                exit(EXIT_FAILURE);
            }

            int isBmp = S_ISREG(fileStat.st_mode) && strstr(entry->d_name, ".bmp");
            int isDir = S_ISDIR(fileStat.st_mode);
            int isLnk = S_ISLNK(fileStat.st_mode);

            writeStatistics(inputPath, outputPath, isBmp, isDir, isLnk);
            exit(EXIT_SUCCESS);
        }
    }

    while (wait(NULL) > 0);

    closedir(dir);
    return 0;
}
