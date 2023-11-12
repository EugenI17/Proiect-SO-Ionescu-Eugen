#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: ./program <fisier_intrare>\n", 35);
        return -1;
    }

    int bmpFile = open(argv[1], O_RDONLY);
    if (bmpFile == -1) {
        perror("Error opening BMP file");
        return -1;
    }

    lseek(bmpFile, 18, SEEK_SET);
    int32_t width, height;
    read(bmpFile, &width, sizeof(width));
    read(bmpFile, &height, sizeof(height));

    struct stat fileStat;
    if (fstat(bmpFile, &fileStat) < 0) {
        perror("Error getting file stats");
        close(bmpFile);
        return -1;
    }

    char buffer[512];
    int length = sprintf(buffer,
        "nume fisier: %s\n"
        "inaltime: %d\n"
        "lungime: %d\n"
        "dimensiune: %ld\n"
        "identificatorul utilizatorului: %d\n"
        "timpul ultimei modificari: %s\n"
        "contorul de legaturi: %ld\n"
        "drepturi de acces user: %c%c%c\n"
        "drepturi de acces grup: %c%c%c\n"
        "drepturi de acces altii: %c%c%c\n",
        argv[1], height, width, fileStat.st_size, fileStat.st_uid,
        ctime(&fileStat.st_mtime), fileStat.st_nlink,
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

    int statFile = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (statFile == -1) {
        perror("Error creating statistics file");
        close(bmpFile);
        return -1;
    }
    write(statFile, buffer, length);

    close(bmpFile);
    close(statFile);

    return 0;
}
