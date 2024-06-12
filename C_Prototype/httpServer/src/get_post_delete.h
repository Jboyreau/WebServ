#ifndef GETPOSTDELETE_H
#define GETPOSTDELETE_H
typedef enum {
    HTML_KEY = 45448,
    HTM_KEY = 34216,
    CSS_KEY = 32571,
    JS_KEY = 23426,
    JSON_KEY = 46852,
    XML_KEY = 40440,
    TXT_KEY = 40832,
    JPG_KEY = 34026,
    JPEG_KEY = 44732,
    PNG_KEY = 36400,
    GIF_KEY = 31930,
    SVG_KEY = 38640,
    ICO_KEY = 33075,
    PDF_KEY = 35168,
    ZIP_KEY = 41358,
    MP4_KEY = 29757,
    WEBM_KEY = 50813,
    MP3_KEY = 29648,
    WAV_KEY = 39746
} FileExtensionKey;

typedef enum {
    GET = 'G',
	POST = 'P',
	DELETE = 'D',
} MethodType;

void get_methode(char* header_body, char *request, int comm_socket_fd);
void post_methode(char* header_body, char *request, int comm_socket_fd);
void delete_methode(char* header_body, char *request, int comm_socket_fd);
void error_methode(char* header_body, char *request, int comm_socket_fd);
#endif
