/* stgen6.cpp - State table generator for Counter type PAQ6 archiver.

(C) 2003-2004 Matt Mahoney. mmahoney@cs.fit.edu
This program is free software distributed without warranty
under the terms of the GNU General Public License,
see http://www.gnu.org/licenses/gpl.txt

A Counter state represents two numbers, n0 and n1, using 8 bits, and
the following rule:

  Initial state is (0, 0)
  If input is a 0, then n0 is incremented and n1 is reduced.
  If input is a 1, then n1 is incremented and n0 is reduced.

with the exception of values in the range 40-255, which are incremented
probabilistically up to a maximum of 255.  The representable values
are 40, 44, 48, 56, 64, 96, 128, 160, 192, 224, 255.

The opposite count is reduced to favor newer data, i.e, if n0 is
incremented then n1 is reduced by the following:

  For 0 <= n1 < 2, n1 is unchanged
  For 2 <= n1 < 25, n1 = n1/2
  for 25 <= n1, n1 = sqrt(n1) + 6, rounded down

There are at most 256 states represented by 8 bits.
For large values of n, an approximate representation is used
with probabilistic increment.  The state table needs the following
mappings from the state s:

  n0
  n1
  next state s00 for input 0 if probabilistic increment fails
  next state s01 for input 0 if probabilistic increment succeeds
  next state s10 for input 1 if probabilistic increment fails
  next state s11 for input 1 if probabilistic increment succeeds
  probability of increment succeeding on input 0 (x 2^32-1)
  probability of increment succeeding on input 1 (x 2^32-1)

Thus the output is 256 records written as a C initialization for
an array of 256 structs with 8 members each.  As an optimization
of the target program, the states are ordered by n0+n1.

The n0 and n1 fields are replaced in the output by get0() and get1()
respectively.  These adjusted counts give higher weights when one
of the counts is small.

*/

#include <cstdio>
#include <vector>
#include <algorithm>
#include <map>
#include <cmath>
using namespace std;

// Round n down to a representable state
int round(int n) {
  if (n<40) return n;
  else if (n<48) return n/4*4;
  else if (n<64) return n/8*8;
  else if (n<255) return n/32*32;
  else return 255;
}

// Return the new value of n after increment
int inc(int n) {
  int i;
  for (i=n; i<1000; ++i)
    if (round(i)>n) break;
  return round(i);
}

// Return the new value of n1 when n0 is incremented
int dec(int n0, int n1) {
  if (n1<2)
    ;
  else if (n1<25)
    n1/=2;
  else
    n1=int(sqrt(double(n1))+6);
  return round(n1);
}

struct E {
  int n0, n1, s00, s01, s10, s11;
  E(int i=0, int j=0):
    n0(i), n1(j), s00(0), s01(0), s10(0), s11(0) {}
  void print(int i=0) const {
    int incn1=inc(n1)-n1;
    int incn0=inc(n0)-n0;
    int get0=n0*2;
    int get1=n1*2;
    if (n0==0) get1*=2;
    else if (n1==0) get0*=2;
    else if (n0>n1) get0/=get1, get1=1;
    else if (n1>n0) get1/=get0, get0=1;
    else get0=get1=1;
    char* msg="";
    if (n0+n1>0 && (s00==0 || s01==0 || s10==0 || s11==0)) msg=" oops!";
    printf("    {%3d,%3d,%3d,%3d,%3d,%3d,%10uu,%10uu}, // %d (%d,%d)%s\n",
      get0, get1, s00, s01, s10, s11,
      incn0?0xffffffff/incn0:0,
      incn1?0xffffffff/incn1:0, i, n0, n1, msg);
  }
};

bool operator<(const E& e1, const E& e2) {
  int v1=e1.n0+e1.n1;
  int v2=e2.n0+e2.n1;
  return v1<v2 || (v1==v2 && e1.n0<e2.n0);
}

int main() {

  // Generate all reachable states in m
  map<E,int> m, m1;  // Current and previous set of reachable states
  map<E,int>::iterator p;
  m[E(0,0)]=1;
  while (m.size()!=m1.size()) {
    m1=m;
    for (p=m1.begin(); p!=m1.end(); ++p) {
      const E& e=p->first;
      m[E(e.n0, dec(e.n0, e.n1))];       // s00
      m[E(inc(e.n0), dec(e.n0, e.n1))];  // s01
      m[E(dec(e.n1, e.n0), e.n1)];       // s10
      m[E(dec(e.n1, e.n0), inc(e.n1))];  // s11
    }
  }

  // Compute next states
  vector<E> v;  // Sorted copy of m
  for (p=m.begin(); p!=m.end(); ++p)
    v.push_back(p->first);
  for (int i=0; i<int(v.size()); ++i) {
    E& e=v[i];
    int n0=e.n0;
    int n1=dec(n0, e.n1);
    for (int j=0; j<v.size(); ++j) if (v[j].n0==n0 && v[j].n1==n1) e.s00=j;
    n0=inc(n0);
    for (int j=0; j<v.size(); ++j) if (v[j].n0==n0 && v[j].n1==n1) e.s01=j;
    n1=e.n1;
    n0=dec(n1, e.n0);
    for (int j=0; j<v.size(); ++j) if (v[j].n0==n0 && v[j].n1==n1) e.s10=j;
    n1=inc(n1);
    for (int j=0; j<v.size(); ++j) if (v[j].n0==n0 && v[j].n1==n1) e.s11=j;
  }

  // Print state table
  for (int i=0; i<int(v.size()); ++i)
    v[i].print(i);
  return 0;
}


