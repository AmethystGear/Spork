// return integer power of base ^ exp
// if the integer power would be fractional (i.e. 1/something), return 0
int ipow(int base, int exp)
{
    // handle the (potentially) fractional case
    if (exp < 0) {
        if (base == 1) {
            return 1; 
        } else if (base == -1) {
            return (exp % 2) ? 1 : -1;
        } else {
            // fractional cases (and 0 base) handled here.
            return 0;
        }
    } else {
        // exponentiation by squaring
        int result = 1;
        while(true)
        {
            if (exp & 1) {
                result *= base;
            }
            exp >>= 1;
            if (!exp) {
                break;
            }
            base *= base;
        }
        return result;
    }
}