/* stategen.cpp - State table generator for Counter type PAQ1 archiver.

(C) 2002 Matt Mahoney. mmahoney@cs.fit.edu
This program is free software distributed without warranty
under the terms of the GNU General Public License,
see http://www.gnu.org/licenses/gpl.txt

A Counter state represents two numbers, n0 and n1, using 8 bits, and
the following rule:

  Initial state is (0, 0)
  If input is a 0, then next state is (n0+1, n1/2+1)
  If input is a 1, then next state is (n0/2+1, n1+1)

with the exception that counts less than 2 are not decreased.
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
of the target program, the states are ordered by n0+n1 (the
hash replacement priority) except that states for which n0 and
n1 are incremented with probability 1 come first (so that a
table lookup and random number generation can be avoided for
low states).
*/

#include <cstdio>
#include <vector>
#include <algorithm>
#include <map>

const int N=24;
const int                    // 11 12 13 14 15 16 17 18 19 20  21  22  23  24  
  val[]={0,1,2,3,4,5,6,7,8,9,10,12,14,16,20,24,28,32,48,64,96,128,256,512,1024},
  dcr[]={0,1,2,2,3,3,4,4,5,5, 6, 7, 8, 9,10,11,12,13,15,17,18, 19, 21, 22, 23};

struct E {
  int n0, n1, s00, s01, s10, s11;
  E(int i=0, int j=0):
    n0(i), n1(j), s00(0), s01(0), s10(0), s11(0) {}
  void print(int i=0) const {
    printf("    {%3d,%3d,%3d,%3d,%3d,%3d,%10uu,%10uu}, // %d\n",
      val[n0], val[n1], s00, s01, s10, s11,
      (n0!=N-1)?0xffffffff/(val[n0+1]-val[n0]):0,
      (n1!=N-1)?0xffffffff/(val[n1+1]-val[n1]):0, i);
  }
};

bool operator<(const E& e1, const E& e2) {
  int v1=val[e1.n0]+val[e1.n1]+100*(e1.n0>9 || e1.n1>9);
  int v2=val[e2.n0]+val[e2.n1]+100*(e2.n0>9 || e2.n1>9);
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
      if (e.n0>10)  m[E(e.n0,   dcr[e.n1])]++;  // s00
      if (e.n0<N-1) m[E(e.n0+1, dcr[e.n1])]++;  // s01
      if (e.n0>10)  m[E(dcr[e.n0], e.n1)  ]++;  // s10
      if (e.n1<N-1) m[E(dcr[e.n0], e.n1+1)]++;  // s11
    }
  }

  // Compute next states
  vector<E> v;  // Sorted copy of m
  for (p=m.begin(); p!=m.end(); ++p)
    v.push_back(p->first);
  for (int i=0; i<int(v.size()); ++i) {
    E& e=v[i];
    int n0=e.n0;
    int n1=dcr[e.n1];
    for (int j=0; j<v.size(); ++j) if (v[j].n0==n0 && v[j].n1==n1) e.s00=j;
    if (n0<N-1) ++n0;
    for (int j=0; j<v.size(); ++j) if (v[j].n0==n0 && v[j].n1==n1) e.s01=j;
    n0=dcr[e.n0];
    n1=e.n1;
    for (int j=0; j<v.size(); ++j) if (v[j].n0==n0 && v[j].n1==n1) e.s10=j;
    if (n1<N-1) ++n1;
    for (int j=0; j<v.size(); ++j) if (v[j].n0==n0 && v[j].n1==n1) e.s11=j;
  }

  // Print state table
  for (int i=0; i<int(v.size()); ++i)
    v[i].print(i);
  return 0;
}

