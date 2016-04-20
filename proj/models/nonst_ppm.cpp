#include "utils/util.cpp"
/*
 * Example model here
 * A NonstationaryPPM model guesses the next bit by finding all
matching contexts of n = 1 to 8 bytes (including the last partial
byte of 0-7 bits) and guessing for each match that the next bit
will be the same with weight n^2/f(age).  The function f(age) decays
the count of 0s or 1s for each context by half whenever there are
more than 2 and the opposite bit is observed.  This is an approximation
of the nonstationary model, weight = 1/(t*variance) where t is the
number of subsequent observations and the variance is tp(1-p) for
t observations and p the probability of a 1 bit given the last t
observations.  The aged counts are stored in a hash table of 8M
contexts.
*/
class NonstationaryPPM: public Model {
  enum {N=8};  // Number of contexts
  int c0;  // Current 0-7 bits of input with a leading 1
  int c1;  // Previous whole byte
  int cn;  // c0 mod 53 (low bits of hash)
  vector<Counter> counter0;  // Counters for context lengths 0 and 1
  vector<Counter> counter1;
  Hashtable<Counter, 24> counter2;  // for lengths 2 to N-1
  Counter *cp[N];  // Pointers to current counters
  U32 hash[N];   // Hashes of last 0 to N-1 bytes
public:
  inline void predict();  // Add to counts of 0s and 1s
  inline void update(int y);   // Append bit y (0 or 1) to model
  NonstationaryPPM();
};

NonstationaryPPM::NonstationaryPPM(): c0(1), c1(0), cn(1),
     counter0(256), counter1(65536) {
  for (int i=0; i<N; ++i) {
    cp[i]=&counter0[0];
    hash[i]=0;
  }
}

void NonstationaryPPM::predict() {

  for (int i=0; i<N; ++i) {
    const int wt=(i+1)*(i+1);
    n0+=cp[i]->get0()*wt;
    n1+=cp[i]->get1()*wt;
  }
}

// Add bit y (0 or 1) to model
void NonstationaryPPM::update(int y) {

  // Count y by context
  for (int i=0; i<N; ++i)
    if (cp[i])
      cp[i]->add(y);

  // Store bit y
  cn+=cn+y;
  if (cn>=53) cn-=53;
  c0+=c0+y;
  if (c0>=256) {  // Start new byte
    for (int i=N-1; i>0; --i)
      hash[i]=(hash[i-1]+c0)*987660757;
    c1=c0-256;
    c0=1;
    cn=1;
  }

  // Set up pointers to next counters
  cp[0]=&counter0[c0];
  cp[1]=&counter1[c0+(c1<<8)];
  for (int i=2; i<N; ++i)
    cp[i]=&counter2[hash[i]+cn+(c0<<24)];
}
