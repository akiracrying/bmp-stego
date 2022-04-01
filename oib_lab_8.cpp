#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <sys\stat.h>
void bin_print(unsigned n) {
    if (n) {
        bin_print(n >> 1);      // бинарный принтф, использовал при гениальном дебаге через принтф
        printf("%u", n & 1);
    }
}
typedef struct {
    int text_mask; 
    int img_mask;
}Masks;

Masks c_masks(int deg) {

    Masks arr;
    arr.text_mask = 0b11111111; // маски на текст
    arr.img_mask = 0b11111111;  // и картинку 
    arr.text_mask <<= (8 - deg);
    arr.text_mask %= 256;       // если слишком длинно, то убираем лишние биты.
    arr.img_mask >>= deg;       // производим сдвиги, чтобы получить из картинки
    arr.img_mask <<= deg;       // необходимые биты для замены

    return arr;
}

int decode() {
    fflush(NULL);
    FILE* decoded_text = fopen("decoded_text.txt", "w");
    FILE* encoded = fopen("encoded.bmp", "rb");
    fseek(decoded_text, 0, SEEK_SET);
    fseek(encoded, 0, SEEK_SET);
    struct stat enc_stat;
    fstat(_fileno(encoded), &enc_stat);

    int deg = 0;
    int symbols = 0;

    printf("enter the amount of bits(1/2/4/8):\n");// ввести сколько битов от исходного 
    scanf("%d", &deg);
    printf("enter the amount of symbols to read:\n");
    scanf("%d", &symbols);

    if (enc_stat.st_size < symbols) {
        printf("the required text is too big\n");
        return EXIT_FAILURE;
    }
    fseek(encoded, 54, SEEK_SET);
    Masks encode_masks = c_masks(deg);
    encode_masks.img_mask = ~encode_masks.img_mask;

    int read = 0;
    char to_show;
    while (read < symbols) {
        int c = 0;
        for (int i = 0; i < 8; i += deg) {
            char i_byte[256];                                   // просто заносим
            fread(&i_byte, 1, 1, encoded);                      // 1 символ в i_byte
            int ch = int(i_byte[0]) & encode_masks.img_mask;    // накладываем маску, чтобы понять, куда заносить
            c <<= deg;
            c |= ch;
        }
        read++;
        to_show =  char(c) ;
        fwrite(&to_show, 1, 1, decoded_text);
    }
    printf("\n");
    fclose(decoded_text);
    fclose(encoded);
}
int encode() {
    FILE* text = fopen("text.txt", "r");
    FILE* image = fopen("main_pic.bmp", "rb"); // rb и wb это считывание побайтно
    FILE* encoded = fopen("encoded.bmp", "wb");
    struct stat text_stat;                     //
    struct stat image_stat;                    // 3 структуры, содержашие информацию о соотв. файлах 
    struct stat enc_stat;                      //

    int deg = 0;
    printf("enter the amount of bits(1/2/4/8):\n"); // ввести сколько битов от исходного 
    scanf("%d", &deg);                              // байта цвета будем шифровать

    fstat(_fileno(text), &text_stat);   // заполнение
    fstat(_fileno(image), &image_stat); // структур
    int deg_8 = deg;                    // это если шифрование == 8
    if (deg == 8)deg_8 = 1;             // то фиксим ошибку при 8%8
    if (image_stat.st_size * (deg_8%8) - 54 < text_stat.st_size) {
        printf("error: the text is too long\n");                 // если размер файла текста больше размера файла img
        return EXIT_FAILURE;                                     // то выводим ошибку и ливаем
    }

    char first54[55];                   // исходя из формата bmp сюда записываются первые 54 байта инфы, содержащие заголовок файла и т.д.
    fseek(image, 0, SEEK_SET);          // ставим указатель в файле на начло (SEEK_SET == начало файла).
    fread(&first54, 54, 1, image);      // читаем первые 54 символа и
    fwrite(&first54, 54, 1, encoded);   // заносим в encoded (заметим, что изменился размер файла)

    Masks encode_masks = c_masks(deg);  // маски из функции
    int n = 0;                          // количество прочитанных символов
    int tell_me = 0;                    // f(x), показывающая где сейчас указатель
    while (1) {                         // цикл вечен
        
        char t_byte[256];                   // поочереди берём символы из 
        fread(&t_byte, 1, 1, text);         // файла .txt.
        if (n == text_stat.st_size) break;  // не вечен ( если дошли до ласт символа)
        int t_byte_ch = int(t_byte[0]);     // получаем аски-значение символа

        for (int i = 0; i < 8; i+= deg) {   // этот цикл для байта, т.е. заполняет байт инфой об 1 символе.
            char i_byte[256];               // просто заносим
            fread(&i_byte, 1, 1, image);    // 1 символ в i_byte

            int ch = int(i_byte[0]) & encode_masks.img_mask;    // накладываем маску, чтобы понять, куда заносить
            int bits = t_byte_ch & encode_masks.text_mask;      // потихоньку берём текст

            bits >>= (8 - deg);              // делаем сдвиг 
            ch |= bits;                      // обычное логчическое ИЛИ которое по факту применяет маску
            i_byte[0] = ch;                  // обратно к норм виду 
            fwrite(&i_byte, 1, 1, encoded);
            t_byte_ch <<= deg;               // сдвигаемся
            tell_me = ftell(image);          // узнаём, где сейчас находимся в файле (указатель на место)
        }
        n++;
    }
    for (int l = 0; l < image_stat.st_size - tell_me; l++) { // отнимаем указатель => заполняем изначальными байтами не учитывая наше сообщение
        unsigned  char owa[256];                             // после того, как полностью перенесли текст,
        fread(&owa, 1, 1, image);                            // нужно полность догрузить остальную картинку.
        fwrite(&owa, 1, 1, encoded);                         // делаем это побитово 
    }
    printf("\n");

    fflush(NULL);
    fclose(text);
    fclose(image);
    fclose(encoded);
}
int main() {
    fflush(NULL);
    int a = 0;
    while (a != 3) {
        printf("choose operation:\n1. encode\n2. decode\n3. exit\n");
        scanf("%d", &a);
        if (a == 1) {
            encode();
        }
        else if (a == 2) {
            decode();
        }
        else if (a == 3) {
            return EXIT_SUCCESS;
        }
        else {
            printf("unknown command\n");
        }
    }
}
