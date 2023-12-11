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
    if (argc != 4) {
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

        int totalSentences = 0;
        char inputPath[1024], outputPath[1024];
        sprintf(inputPath, "%s/%s", argv[1], entry->d_name);
        sprintf(outputPath, "%s/%s_statistica.txt", argv[2], entry->d_name);

        struct stat fileStat;
        if (stat(inputPath, &fileStat) < 0) {
            perror("Error getting file stats");
            continue;
        }
        int isBmp = S_ISREG(fileStat.st_mode) && strstr(entry->d_name, ".bmp") != NULL;
        int isDir = S_ISDIR(fileStat.st_mode);
        int isLnk = S_ISLNK(fileStat.st_mode);
        writeStatistics(inputPath, outputPath, isBmp, isDir, isLnk);

        if (S_ISREG(fileStat.st_mode)) {
            int fd1[2], fd2[2];
            if (pipe(fd1) == -1 || pipe(fd2) == -1) {
                perror("Error creating pipes");
                continue;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("Error creating first child process");
                close(fd1[0]);
                close(fd1[1]);
                close(fd2[0]);
                close(fd2[1]);
                continue;
            }

            if (pid == 0) {
                close(fd1[0]);
                int filefd = open(inputPath, O_RDONLY);
                if (filefd < 0) {
                    perror("Error opening file");
                    close(fd1[1]);
                    exit(EXIT_FAILURE);
                }

                char buffer[1024];
                ssize_t bytesRead;
                while ((bytesRead = read(filefd, buffer, sizeof(buffer))) > 0) {
                    if (write(fd1[1], buffer, bytesRead) != bytesRead) {
                        perror("Error writing to pipe");
                        close(filefd);
                        close(fd1[1]);
                        exit(EXIT_FAILURE);
                    }
                }

                close(filefd);
                close(fd1[1]);
                exit(EXIT_SUCCESS);
            } else {
                close(fd1[1]);

                char *isBmp = strstr(entry->d_name, ".bmp");
                char *isDS_Store = strstr(entry->d_name, ".DS_Store");
                if (isBmp == NULL && isDS_Store == NULL) {
                    pid_t pid2 = fork();
                    if (pid2 == -1) {
                        perror("Error creating second child process");
                        close(fd1[0]);
                        close(fd2[0]);
                        close(fd2[1]);
                        continue;
                    }

                    if (pid2 == 0) {
                        dup2(fd1[0], STDIN_FILENO);
                        dup2(fd2[1], STDOUT_FILENO);
                        execl("/Users/macbookpro/Projects/Proiect-SO-Ionescu-Eugen/script.sh", "script.sh", argv[3], NULL);

                        perror("Error executing script");
                        exit(EXIT_FAILURE);
                    } else {
                        close(fd1[0]);
                        close(fd2[1]);

                        int status = 0;
                        waitpid(pid2, &status, 0);
                        char count[4];
                        read(fd2[0], &count, sizeof(count));
                        totalSentences += atoi(count);
                        if( strstr(entry->d_name, ".bmp") == NULL)
                            printf("In fisierul %s au fost identificate in total %d propozitii corecte care contin caracterul %c\n", entry->d_name, totalSentences, argv[3][0]);

                        close(fd2[0]);
                    }
                } else {
                    close(fd1[0]);
                    close(fd2[0]);
                    close(fd2[1]);
                }
            }
        }

}

    closedir(dir);

    return EXIT_SUCCESS;
}
