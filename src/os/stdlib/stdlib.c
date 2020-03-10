#include <stdlib.h>

#include <ctype.h>

char* uitoa(size_t num, char* buf, size_t base, bool prepend_zeros)
{
	if (base < 2)
		return 0;

	size_t i = 0;
	do {
		size_t r = num % base;
		buf[i++] = (r > 9) ? (r-10) + 'A' : r + '0';
		num /= base;
	} while (num > 0 || prepend_zeros && ((base == 16 && i < 8) || (base == 2 && i < 32)));

	size_t end = i-1;
	for (size_t k = 0; k <= end/2; k++) {
		char c = buf[k];
		buf[k] = buf[end-k];
		buf[end-k] = c;
	}

	buf[end+1] = 0;
	return buf;
}

char* itoa(int num, char* buf, size_t base, bool prepend_zeros)
{
	if (base < 2)
		return 0;

	if (num < 0) {
		*buf = '-';
		num *= -1;
	}

	uitoa(num, buf+1, base, prepend_zeros);
	return buf;
}

unsigned long long strtoull(const char *str, char **str_end, int base)
{
	// Basis darf nur 0 oder 2-36 sein
	if (base < 0 || base == 1 || base > 36)
		return 0;

	if (!str)
		return 0;

	unsigned long long num = 0;

	// Vorangehende Leerzeichen ueberspringen
	while(isspace(str[0]))
		str++;

	// Autmatisch Basis erkennen
	if (base == 0)
	{
		// Faengt mit 0 an
		if (str[0] == '0')
		{
			str++;

			// Faengt mit 0x an, also Hexadezimal
			if (tolower(str[0]) == 'x')
			{
				str++;
				base = 16;
			}
			else // Ansonsten Oktal
				base = 8;
		}
		else // Ansonsten Dezimal
			base = 10;
	}

	// Den restlichen String parsen
	while(str)
	{
		// Bei invalidem Zeichen abbrechen
		if (!isalnum(str[0]))
			break;

		int value;

		// Wert der Ziffer berechnen
		if (isdigit(str[0]))
			value = (int)(str[0] - '0');
		else
			value = (int)(toupper(str[0]) - 'A') + 10;

		// Abbrechen wenn Wert nicht im Zahlensystem vorhanden
		if (value >= base)
			break;

		// Zahl um eine Ziffer verschieben und neue Ziffer hinzufuegen
		num *= (unsigned long long)base;
		num += (unsigned long long)value;

		str++;
	}

	// Das Ende der Zahl abspeichern
	if (str_end)
		*str_end = (char*)str;

	return num;
}

unsigned long strtoul(const char *str, char **str_end, int base)
{
	return (unsigned long)strtoull(str, str_end, base);
}

long long strtoll(const char *str, char **str_end, int base)
{
	// Vorangehende Leerzeichen ueberspringen
	while(isspace(str[0]))
		str++;

	// Minuszeichen entfernen
	long long sign = 1;
	if (str[0] == '-')
	{
		str++;
		sign = -1;
	}

	// Geparste Zahl mit Vorzeichen multiplizieren
	return sign * (long long)strtoull(str, str_end, base);
}

long strtol(const char *str, char **str_end, int base)
{
	return (long)strtoll(str, str_end, base);
}

long long atoll(const char *str)
{
	return strtoll(str, NULL, 10);
}

long atol(const char *str)
{
	return strtol(str, NULL, 10);
}

int atoi(const char *str)
{
	return (int)strtol(str, NULL, 10);
}
