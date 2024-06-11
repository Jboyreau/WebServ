void itoa(long long int value, char* buffer)
{
	char* ptr = buffer;
	char* ptr1 = buffer;
	char tmp_char;
	long long int tmp_value;

	if (value < 0) {
		value = -value;
		*ptr++ = '-';
	}
	do
	{
		tmp_value = value;
		value /= 10;
		*ptr++ = "0123456789"[tmp_value - value * 10];
	}
	while (value);
	*ptr-- = '\0';
	if (buffer[0] == '-')
		ptr1++;
	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
}
