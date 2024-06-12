#include <stdio.h>
#include <stdlib.h>

#define EXT_NUM 19

int keygen(char *ext)
{
	int sum = 0;
	int i;

	for (i = 1; *(ext + i); ++i)
		sum += *(ext + i);
	return sum * *(ext + 1);
}

int main(void)
{
	char *extensions[EXT_NUM] = {
		".html",
		".htm",
		".css",
		".js",
		".json",
		".xml",
		".txt",
		".jpg",
		".jpeg",
		".png",
		".gif",
		".svg",
		".ico",
		".pdf",
		".zip",
		".mp4",
		".webm",
		".mp3",
		".wav"
	};
	int keys[EXT_NUM];
	for(int i = 0; i < EXT_NUM; ++i)
		*(keys + i) = keygen(*(extensions + i));
	for(int i = 0; i < EXT_NUM; ++i)
		printf("%s = %d\n", extensions[i], keys[i]);
	for(int i = 0; i < EXT_NUM; ++i)
	{
		for (int j = i + 1; j < EXT_NUM; ++j)
		{
			if (*(keys + i) == *(keys + j))
			{
				printf("Failure at %s & %s value = %d.\n", extensions[i], extensions[j], keys[i]);
				return 0;
			}
		}
	}
	printf("SUCCESS\n");
	return 0;
}

/*
Types de Fichiers et Content-Types

    1.HTML
        Content-Type: text/html
        Extensions: .html, .htm

    2.CSS
        Content-Type: text/css
        Extensions: .css

    3.JavaScript
        Content-Type: application/javascript
        Extensions: .js

    4.JSON
        Content-Type: application/json
        Extensions: .json

    5.XML
        Content-Type: application/xml
        Extensions: .xml

    6.Texte
        Content-Type: text/plain
        Extensions: .txt

    7.JPEG
        Content-Type: image/jpeg
        Extensions: .jpg, .jpeg

    8.PNG
        Content-Type: image/png
        Extensions: .png

    9.GIF
        Content-Type: image/gif
        Extensions: .gif

    10.SVG
        Content-Type: image/svg+xml
        Extensions: .svg

    11.ICO
        Content-Type: image/x-icon
        Extensions: .ico

    12.PDF
        Content-Type: application/pdf
        Extensions: .pdf

    13.ZIP
        Content-Type: application/zip
        Extensions: .zip

    14.MP4
        Content-Type: video/mp4
        Extensions: .mp4

    15.WebM
        Content-Type: video/webm
        Extensions: .webm

    16.MP3
        Content-Type: audio/mpeg
        Extensions: .mp3

    17.WAV
        Content-Type: audio/wav
        Extensions: .wav
*/
