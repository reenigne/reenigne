#include <stdio.h>
#include <stdint.h>

bool isPrime(int64_t p)
{
    for (int64_t x = 2; x*x <= p; ++x)
        if ((p%x) == 0)
            return false;
    return true;
}

int main()
{
    FILE* out = fopen("primes.raw", "wb");
    int64_t primes[1200];
    int64_t p = 2;
    int64_t y = 0;
    for (int64_t p = 2;; ++p) {
        if (isPrime(p)) {
            primes[y++] = p;
            if (y == 1200)
                break;
        }
    }

    for (int64_t y = 0; y < 1200; ++y) {
        int64_t xx = 0;
        for (int64_t x = primes[y];; ++x) {
            bool sieved = false;
            for (int64_t z = 0; z < y; ++z)
                if (x % primes[z] == 0) {
                    sieved = true;
                    break;
                }
            if (sieved)
                continue;
                    
            if (isPrime(x))
                fputc(0, out);
            else
                fputc(255, out);
            ++xx;
            if (xx == 1600)
                break;
        }
    }
    fclose(out);
}
