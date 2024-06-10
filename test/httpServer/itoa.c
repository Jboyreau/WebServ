void itoa(long long int value, char* buffer) {
    // Pointeurs pour la conversion
    char* ptr = buffer;
    char* ptr1 = buffer;
    char tmp_char;
    long long int tmp_value;

    // Vérifier la valeur pour le signe
    if (value < 0) {
        value = -value;
        *ptr++ = '-';
    }

    // Convertir le nombre en chaîne
    do {
        tmp_value = value;
        value /= 10;
        *ptr++ = "0123456789"[tmp_value - value * 10];
    } while (value);

    // Terminer la chaîne
    *ptr-- = '\0';

    // Inverser la chaîne
    if (buffer[0] == '-') {
        ptr1++;
    }
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}
