#include "datatypes.h"
#include <vector>

/* A Predictor predicts the next bit given the bits so far using a
collection of models.  Methods:

   p() returns probability of a 1 being the next bit, P(y = 1)
     as a 16 bit number (0 to 64K-1).
   update(y) updates the models with bit y (0 or 1)
*/

class Predictor {
//  NonstationaryPPM m1;
//  MatchModel m2;
//  WordModel m3;
//  CyclicModel m4;
public:
  U16 p();
  void update(int y); 
};

