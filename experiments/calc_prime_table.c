#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

bool is_prime(size_t x){
	for(size_t i = 3; true; i += 2){
		size_t q = x / i;
		if (q < i)
			return true;
		if (x % i == 0)
			return false;
	}
	
	return true;
}

size_t snap_to_prime(size_t x){
	// No match, compute next larger prime
	// Make x odd by setting the least significant bit (there are no even primes)
	x |= 1;
	
	// Loop odd numbers until we have a prime
	while( !is_prime(x) )
		x += 2;
	
	return x;
}

void main(){
	printf("\n{");
	for(size_t i = 5; i < 20000000; i *= 2){
		i = snap_to_prime(i);
		printf("%zu, ", i);
	}
	printf("}\n");
}