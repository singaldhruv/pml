#include "predictor.h"
U16 
Predictor::p() {
    int n0=1, n1=n0;
//    m1.predict(n0, n1);
//    m2.predict(n0, n1);
//    m3.predict(n0, n1);
//    m4.predict(n0, n1);
//    return a function of the model predictions
//    CALL EXTERNAL HERE?
    return U16(65535.0*n1/(n0+n1));
}

void 
Predictor::update(int y) {
//    m1.update(y);
//    m2.update(y);
//    m3.update(y);
//    m4.update(y);
}

