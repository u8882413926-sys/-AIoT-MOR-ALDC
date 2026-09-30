


    // !!! This file is generated using emlearn !!!

    #include <stdint.h>
    

static inline int32_t anomaly_model_tree_0(const float *features, int32_t features_length) {
          if (features[0] < 0.646282f) {
              if (features[0] < 0.416100f) {
                  if (features[5] < 0.510714f) {
                      return 1;
                  } else {
                      if (features[5] < 0.524487f) {
                          return 0;
                      } else {
                          return 1;
                      }
                  }
              } else {
                  if (features[6] < -0.005091f) {
                      if (features[5] < 0.890972f) {
                          if (features[4] < 0.384003f) {
                              return 0;
                          } else {
                              return 1;
                          }
                      } else {
                          return 1;
                      }
                  } else {
                      if (features[0] < 0.465812f) {
                          if (features[7] < 0.065470f) {
                              return 1;
                          } else {
                              return 0;
                          }
                      } else {
                          if (features[0] < 0.630048f) {
                              if (features[7] < 0.188308f) {
                                  return 0;
                              } else {
                                  return 0;
                              }
                          } else {
                              if (features[1] < 0.240002f) {
                                  return 0;
                              } else {
                                  return 1;
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[6] < 0.000836f) {
                  return 1;
              } else {
                  if (features[4] < 0.559537f) {
                      return 1;
                  } else {
                      return 0;
                  }
              }
          }
        }
        

int32_t anomaly_model_predict(const float *features, int32_t features_length) {

        int32_t votes[2] = {0,};
        int32_t _class = -1;

        _class = anomaly_model_tree_0(features, features_length); votes[_class] += 1;
    
        int32_t most_voted_class = -1;
        int32_t most_voted_votes = 0;
        for (int32_t i=0; i<2; i++) {

            if (votes[i] > most_voted_votes) {
                most_voted_class = i;
                most_voted_votes = votes[i];
            }
        }
        return most_voted_class;
    }
    

int anomaly_model_predict_proba(const float *features, int32_t features_length, float *out, int out_length) {

        int32_t _class = -1;

        for (int i=0; i<out_length; i++) {
            out[i] = 0.0f;
        }

        _class = anomaly_model_tree_0(features, features_length); out[_class] += 1.0f;
    
        // compute mean
        for (int i=0; i<out_length; i++) {
            out[i] = out[i] / 1;
        }
        return 0;
    }
    