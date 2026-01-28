#ifndef DIFFIE_HELLMAN_H
#define DIFFIE_HELLMAN_H

#include <cstdint>
#include <random>

class DiffieHellman {
private:
    uint64_t prime;      // большое простое число p
    uint64_t generator;  // генератор g
    uint64_t privateKey; // секретный ключ a
    uint64_t publicKey;  // публичный ключ A

    // Быстрое возведение в степень по модулю
    uint64_t modPow(uint64_t base, uint64_t exp, uint64_t mod) const {
        uint64_t result = 1;
        base %= mod;
        while (exp > 0) {
            if (exp & 1) result = (result * base) % mod;
            base = (base * base) % mod;
            exp >>= 1;
        }
        return result;
    }

public:
    // Конструктор: передаём простое число p и генератор g
    DiffieHellman(uint64_t p, uint64_t g) : prime(p), generator(g) {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(2, prime - 2);

        privateKey = dis(gen);           // случайный секретный ключ
        publicKey = modPow(generator, privateKey, prime); // вычисляем публичный
    }

    uint64_t getPublicKey() const { return publicKey; }
    uint64_t getPrime() const { return prime; }
    uint64_t getGenerator() const { return generator; }

    // Вычисление общего секрета по чужому публичному ключу
    uint64_t computeSharedKey(uint64_t otherPublic) const {
        return modPow(otherPublic, privateKey, prime);
    }
};

#endif // DIFFIE_HELLMAN_H
