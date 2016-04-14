/* Model interface.  A Predictor is made up of a collection of various
models, whose outputs are summed to yield a prediction.  Methods:

   Model.predict(int& n0, int& n1) - Adds to counts n0 and n1 such that
     it predicts the next bit will be a 1 with probability n1/(n0+n1)
     and confidence n0+n1.
   Model.update(int y) - Appends bit y (0 or 1) to the model.
*/
class Model {
public:
  virtual void predict() const = 0;
  virtual void update(int y) = 0;
  virtual ~Model() {}
};

