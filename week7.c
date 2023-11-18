#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

void writeStatistics(const char *name, struct stat *fileStat, int statFile, int isBmp, int isDir, int isLnk) {
    char buffer[2048];
    int length = 0;

    if (isBmp) {
        int bmpFile = open(name, O_RDONLY);
        if (bmpFile == -1) {
            perror("Error opening BMP file");
            return;
        }

        lseek(bmpFile, 18, SEEK_SET);
        int32_t width, height;
        read(bmpFile, &width, sizeof(width));
        read(bmpFile, &height, sizeof(height));
        close(bmpFile);

        length = sprintf(buffer,
            "nume fisier: %s\n"
            "inaltime: %d\n"
            "lungime: %d\n",
            name, height, width);
    } else if (isDir) {
        length = sprintf(buffer,
            "nume director: %s\n",
            name);
    } else if (isLnk) {
        length = sprintf(buffer,
            "nume legatura: %s\n",
            name);
    } else {
        length = sprintf(buffer, "nume fisier: %s\n", name);
    }

    length += sprintf(buffer + length,
        "dimensiune: %ld\n"
        "identificatorul utilizatorului: %d\n"
        "timpul ultimei modificari: %s"
        "contorul de legaturi: %ld\n"
        "drepturi de acces user: %c%c%c\n"
        "drepturi de acces grup: %c%c%c\n"
        "drepturi de acces altii: %c%c%c\n",
        fileStat->st_size,
        fileStat->st_uid,
        ctime(&fileStat->st_mtime),
        fileStat->st_nlink,
        (fileStat->st_mode & S_IRUSR) ? 'R' : '-',
        (fileStat->st_mode & S_IWUSR) ? 'W' : '-',
        (fileStat->st_mode & S_IXUSR) ? 'X' : '-',
        (fileStat->st_mode & S_IRGRP) ? 'R' : '-',
        (fileStat->st_mode & S_IWGRP) ? 'W' : '-',
        '-',
        (fileStat->st_mode & S_IROTH) ? 'R' : '-',
        (fileStat->st_mode & S_IWOTH) ? 'W' : '-',
        '-'
    );

    write(statFile, buffer, length);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: ./program <director_intrare>\n", 37);
        return -1;
    }

    DIR *dir = opendir(argv[1]);
    if (dir == NULL) {
        perror("Error opening directory");
        return -1;
    }

    int statFile = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (statFile == -1) {
        perror("Error creating statistics file");
        closedir(dir);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char path[1024];
        sprintf(path, "%s/%s", argv[1], entry->d_name);

        struct stat fileStat;
        if (lstat(path, &fileStat) < 0) {
            perror("Error getting file stats");
            continue;
        }

        if (S_ISREG(fileStat.st_mode)) {
            char *ext = strrchr(entry->d_name, '.');
            int isBmp = ext && !strcmp(ext, ".bmp");
            writeStatistics(entry->d_name, &fileStat, statFile, isBmp, 0, 0);
        } else if (S_ISDIR(fileStat.st_mode)) {
            writeStatistics(entry->d_name, &fileStat, statFile, 0, 1, 0);
        } else if (S_ISLNK(fileStat.st_mode)) {
            writeStatistics(entry->d_name, &fileStat, statFile, 0, 0, 1);
        }
    }

    closedir(dir);
    close(statFile);
    return 0;
}
