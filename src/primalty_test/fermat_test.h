// primality_test/fermat_test.h
#pragma once

#include "primality_test.h" 
#include "prng.h"           
#include <boost/multiprecision/cpp_int.hpp> /
#include <stdexcept>        

#include <boost/multiprecision/number.hpp>
#include <algorithm> 

using BigInt = boost::multiprecision::cpp_int;

class FermatTest : public PrimalityTest
{
    private:
        //Gera um numero entre 1 < a < n-1
        //Ensure that a is in the range [1, n-1]
        BigInt generate_witness(const BigInt& n, PRNG& rng);

    public:

        FermatTest() = default;

        ~FermatPrimalityTest() override = default;


        bool isPrime(const BigInt& n, int iterations, PRNG& rng) override;
}
