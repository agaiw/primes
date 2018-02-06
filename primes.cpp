#include <iostream>
#include <thread>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
#include <chrono>
#include <cmath>
#include <gmpxx.h>
#include <random>

#define ACCURACY 15
std::ifstream openFile(std::string filename);

std::chrono::duration<double> primesTrialDivision(std::vector<long int> numbers, int threadId, int cores);
std::chrono::duration<double> primesMillerRabin(std::vector<long int> numbers, int threadId, int cores);
std::chrono::duration<double> primesLibGmp(std::vector<long int> numbers, int threadId, int cores);

long int mulmod(long int a, long int b, long int mod);
long int modulo(long int b, long int exp, long int mod);

bool isPrimeTrialDivision(long int n);
bool isPrimeMillerRabin(long int n);

// sum of found prime numbers for all threads
// to verify if algorithms give the same results
int countTD = 0;
int countMR = 0;
int countGMP = 0;

int main(int argc, const char** argv) {
  unsigned cores = std::thread::hardware_concurrency();
  std::chrono::duration<double> totalTD (0);
  std::chrono::duration<double> totalMR (0);
  std::chrono::duration<double> totalGMP (0);

    if(argc != 2) {
      perror("Please provide filename.");
      return 1;
    }
  // read numbers from file to matrix
  std::ifstream f = openFile(argv[1]);
  std::vector<long int> numbers;
  std::string line;
  while(getline(f, line)) {
    numbers.push_back(std::stol(line));
  }
  f.close();

  std::mutex results;
  std::vector<std::thread> threads(cores);
  for (unsigned i = 0; i < cores; ++i) {
    threads[i] = std::thread([&results, i, numbers, &totalTD, &totalMR, &totalGMP, cores] {

      std::chrono::duration<double> tempTD = primesTrialDivision(numbers, i, cores);
      results.lock();
      totalTD += tempTD;
      results.unlock(); 

      std::chrono::duration<double> tempMR = primesMillerRabin(numbers, i, cores);
      results.lock();
      totalMR += tempMR;
      results.unlock(); 

      std::chrono::duration<double> tempGMP = primesLibGmp(numbers, i, cores);
      results.lock();
      totalGMP += tempGMP;
      results.unlock();
    });
  }

  for (auto& t : threads) {
    t.join();
  }
  std::cout << "Trial Division results: " << std::flush;
  std::cout << "elapsed time " << totalTD.count() << ", found " << countTD << " prime numbers." << std::endl;
  std::cout << "Miller Rabin results: " << std::flush;
  std::cout << "elapsed time " << totalMR.count() << ", found " << countMR << " prime numbers." << std::endl;
  std::cout << "GMP library results: " << std::flush;
  std::cout << "elapsed time " << totalGMP.count() << ", found " << countGMP << " prime numbers." << std::endl;
  return 0;
}

std::chrono::duration<double> primesTrialDivision(std::vector<long int> numbers, int threadId, int cores) {
  auto start = std::chrono::system_clock::now(); 
  for (std::size_t element = 0; element < numbers.size(); ++element) {
    if (element % cores == threadId) {
      if (isPrimeTrialDivision(numbers.at(element))) {
        countTD++;
        }
    }
  }
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  return elapsed;
};

std::chrono::duration<double> primesMillerRabin(std::vector<long int> numbers, int threadId, int cores) {
  auto start = std::chrono::system_clock::now(); 
  for (std::size_t element = 0; element < numbers.size(); ++element) {
    if (element % cores == threadId) {
      if (isPrimeMillerRabin(numbers.at(element))) {
        countMR++;
      }
    }
  }
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  return elapsed;
};

std::chrono::duration<double> primesLibGmp(std::vector<long int> numbers, int threadId, int cores) {
  auto start = std::chrono::system_clock::now(); 
  for (std::size_t element = 0; element < numbers.size(); ++element) {
    if (element % cores == threadId) {
      mpz_t number;
      mpz_init(number);
      mpz_set_ui(number, numbers.at(element));
      if (mpz_probab_prime_p(number, ACCURACY) > 0) {
        countGMP++;
      }
    }
  }
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  return elapsed;
};

std::ifstream openFile(std::string filename) {
  std::ifstream f (filename);
    if (!f.is_open())
        perror(("error while opening file " + filename).c_str());
    if (f.bad())
        perror(("error while reading file " + filename).c_str());
  return f; 
};

bool isPrimeTrialDivision(long int n) {
  if (n <= 1) { return false; }
  if (n == 2) { return true; }

  for (long int i = 2; i <= sqrt(n); ++i)
  {
    if (n % i == 0) { return false; }
  }
  return true;
}

long int mulmod(long int a, long int b, long int mod) {
  long int x = 0;
  long int y = a % mod;
  while (b > 0) {
    if (b % 2 == 1) { x = (x + y) % mod; }
        y = (y * 2) % mod;
        b /= 2;
  }
  return x % mod;
}

long int modulo(long int b, long int exp, long int mod) {
  long int x = 1;
  long int y = b;
  while (exp > 0) {
        if (exp % 2 == 1) { x = mulmod(x, y, mod); }
        y = mulmod(y, y, mod);
        exp /= 2;
    }
    return x % mod;
}

bool isPrimeMillerRabin(long int p) {
  if (p < 2) { return false; }
    if (p != 2 && p % 2 == 0) { return false; }
    long int s = p - 1;
    while (s % 2 == 0) { s /= 2; }
    for (int i = 0; i < ACCURACY; i++) {
      long int a = rand() % (p - 1) + 1;
      long int temp = s;
      long int mod = modulo(a, temp, p);
      if (mod == 1 || mod == p - 1) { continue; }
      while (temp != (p - 1) && mod != 1 && mod != (p - 1)) {
        mod = mulmod(mod, mod, p);
        temp *= 2;
      }
      if (mod != (p - 1)) { return false; }  
     }
     return true;
}
