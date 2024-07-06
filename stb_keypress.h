#ifndef STB_KEYPRESS_H
#define STB_KEYPRESS_H
    char GetKeyPress();
#endif // STB_KEYPRESS_H

#ifdef STB_KEYPRESS_IMPLEMENTATION
#if defined(_WIN32) || defined(_WIN64)
    #include <conio.h>
    char GetKeyPress() {
        return _getch();
    }
#else
    #include <unistd.h>
    #include <termios.h>
    char GetKeyPress() {
        struct termios oldTermios;
        char ch;
        struct termios newTermios;
        tcgetattr(STDIN_FILENO, &oldTermios);
        newTermios = oldTermios;
        newTermios.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
        return ch;
    }
#endif
#endif // STB_KEYPRESS_IMPLEMENTATION
